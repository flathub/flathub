use crate::{
    audio::{
        decode,
        playback::{LoadedStems, StemSet},
    },
    cache,
    commands::remote_library,
    library::Song,
    library_root::LibraryRoot,
    media_g::{self, MEDIA_G_ZIP},
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use std::path::Path;

pub(crate) struct PlaybackSourceLoad {
    pub(crate) decoded_audio: decode::DecodedAudio,
    pub(crate) stems: Option<LoadedStems>,
}

pub(crate) fn probe_song_audio(library_root: &LibraryRoot, song: &Song) -> Result<()> {
    let song_path = resolve_song_file_path(song)?;
    let absolute_path = library_root.resolve(song_path);
    if song.media_g_container.as_deref() == Some(MEDIA_G_ZIP) {
        let asset = media_g::inspect_zip_for_media_g(&absolute_path)?;
        return decode::probe_bytes(asset.audio_bytes, &asset.audio_extension)
            .with_context(|| format!("failed to probe audio for {}", song_path));
    }

    decode::probe_file(&absolute_path)
        .with_context(|| format!("failed to probe audio for {}", song_path))
}

pub(crate) fn load_song_audio(library_root: &LibraryRoot, song: &Song) -> Result<decode::DecodedAudio> {
    let song_path = resolve_song_file_path(song)?;
    let absolute_path = library_root.resolve(song_path);
    if song.media_g_container.as_deref() == Some(MEDIA_G_ZIP) {
        let asset = media_g::inspect_zip_for_media_g(&absolute_path)?;
        return decode::decode_bytes(asset.audio_bytes, &asset.audio_extension)
            .with_context(|| format!("failed to decode audio for {}", song_path));
    }

    decode::decode_file(&absolute_path)
        .with_context(|| format!("failed to decode audio for {}", song_path))
}

pub(crate) fn load_playback_source(
    app_data_dir: Option<&Path>,
    connection: &Connection,
    library_root: &LibraryRoot,
    song: &Song,
) -> Result<PlaybackSourceLoad> {
    if song.is_remote_stems() {
        return load_remote_stems_playback_source(connection, library_root, song);
    }

    if song.is_remote() {
        ensure_remote_song_files_cached(app_data_dir, song)?;
    }

    Ok(PlaybackSourceLoad {
        decoded_audio: load_song_audio(library_root, song)?,
        stems: None,
    })
}

pub(crate) fn load_cached_stems_for_song(
    app_data_dir: Option<&Path>,
    connection: &Connection,
    library_root: &LibraryRoot,
    song: &Song,
) -> Result<LoadedStems> {
    if song.is_remote_stems() {
        ensure_remote_stem_files_cached(app_data_dir, connection, song)?;
        return load_remote_stems_playback_source(connection, library_root, song)?
            .stems
            .context("remote stems song did not yield attached stems");
    }

    let cached = cache::stems::get_cached_stem_entry(connection, &song.hash)
        .context("failed to load cached stems")?
        .with_context(|| format!("no cached stems for song {}", song.hash))?;

    decode_stem_entry(library_root, &cached)
}

fn resolve_song_file_path(song: &Song) -> Result<&str> {
    song.file_path
        .as_deref()
        .with_context(|| format!("song {} does not have a local file path", song.hash))
}

fn ensure_remote_song_files_cached(app_data_dir: Option<&Path>, song: &Song) -> Result<()> {
    let Some(app_data_dir) = app_data_dir else {
        return Ok(());
    };
    if let Some(file_path) = song.file_path.as_deref() {
        remote_library::ensure_remote_file_cached(app_data_dir, file_path)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    }
    if let Some(cdg_path) = song.cdg_path.as_deref() {
        remote_library::ensure_remote_file_cached(app_data_dir, cdg_path)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    }
    Ok(())
}

fn ensure_remote_stem_files_cached(
    app_data_dir: Option<&Path>,
    connection: &Connection,
    song: &Song,
) -> Result<()> {
    let Some(app_data_dir) = app_data_dir else {
        return Ok(());
    };
    let Some(cached) = cache::stems::get_cached_stem_entry(connection, &song.hash)
        .context("failed to load cached stems")?
    else {
        return Ok(());
    };

    remote_library::ensure_remote_file_cached(app_data_dir, &cached.vocals_path)
        .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    remote_library::ensure_remote_file_cached(app_data_dir, &cached.accomp_path)
        .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    if let Some(drums_path) = cached.drums_path.as_deref() {
        remote_library::ensure_remote_file_cached(app_data_dir, drums_path)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    }
    if let Some(bass_path) = cached.bass_path.as_deref() {
        remote_library::ensure_remote_file_cached(app_data_dir, bass_path)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    }
    if let Some(other_path) = cached.other_path.as_deref() {
        remote_library::ensure_remote_file_cached(app_data_dir, other_path)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    }
    Ok(())
}

fn load_remote_stems_playback_source(
    connection: &Connection,
    library_root: &LibraryRoot,
    song: &Song,
) -> Result<PlaybackSourceLoad> {
    let cached = cache::stems::get_cached_stem_entry(connection, &song.hash)
        .context("failed to load cached stems")?
        .with_context(|| format!("no cached stems for song {}", song.hash))?;

    if cached.has_individual_stems() {
        let LoadedStems::FourStem(StemSet {
            vocals,
            drums,
            bass,
            other,
        }) = decode_stem_entry(library_root, &cached)?
        else {
            unreachable!("individual stem cache entries decode to four stems");
        };
        Ok(PlaybackSourceLoad {
            decoded_audio: vocals.clone(),
            stems: Some(LoadedStems::FourStem(StemSet {
                vocals,
                drums,
                bass,
                other,
            })),
        })
    } else {
        let LoadedStems::TwoStem {
            vocals,
            accompaniment,
        } = decode_stem_entry(library_root, &cached)?
        else {
            unreachable!("two stem cache entries decode to two stems");
        };
        Ok(PlaybackSourceLoad {
            decoded_audio: accompaniment.clone(),
            stems: Some(LoadedStems::TwoStem {
                vocals,
                accompaniment,
            }),
        })
    }
}

fn decode_stem_entry(
    library_root: &LibraryRoot,
    cached: &cache::stems::StemCacheEntry,
) -> Result<LoadedStems> {
    let load_stem = |path: &str| -> Result<decode::DecodedAudio> {
        let abs = library_root.resolve(path);
        decode::decode_file(&abs).with_context(|| format!("failed to decode stem {}", path))
    };

    if cached.has_individual_stems() {
        Ok(LoadedStems::FourStem(StemSet {
            vocals: load_stem(&cached.vocals_path)?,
            drums: load_stem(cached.drums_path.as_deref().context("missing drums stem path")?)?,
            bass: load_stem(cached.bass_path.as_deref().context("missing bass stem path")?)?,
            other: load_stem(cached.other_path.as_deref().context("missing other stem path")?)?,
        }))
    } else {
        Ok(LoadedStems::TwoStem {
            vocals: load_stem(&cached.vocals_path)?,
            accompaniment: load_stem(&cached.accomp_path)?,
        })
    }
}
