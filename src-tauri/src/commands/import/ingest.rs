use super::expand::match_cdg_source;
use crate::{
    cache,
    commands::error::current_unix_timestamp,
    hash,
    library::Song,
    library_root::LibraryRoot,
    lyrics::fetch::read_embedded_lyrics,
    media_g::{self, MEDIA_G_PAIRED, MEDIA_G_ZIP},
    metadata,
};
use anyhow::{Context, Result};
use lofty::{file::TaggedFileExt, tag::ItemKey};
use rusqlite::Connection;
use sha2::{Digest, Sha256};
use std::{
    collections::{HashMap, HashSet},
    fs::{self, File},
    io::Read,
    path::{Path, PathBuf},
};

use crate::lyrics::fetch::LyricsSource;

pub(super) fn build_and_store_song(
    source: &Path,
    library: &LibraryRoot,
    selected_cdg_by_stem: &HashMap<String, Vec<PathBuf>>,
    explicit_cdg_by_audio_path: &HashMap<String, String>,
    skip_cdg_for_audio_paths: &[String],
    consumed_cdg_paths: &mut HashSet<PathBuf>,
) -> Result<Song> {
    if let Some(cdg_source) = match_cdg_source(
        source,
        selected_cdg_by_stem,
        explicit_cdg_by_audio_path,
        skip_cdg_for_audio_paths,
    ) {
        consumed_cdg_paths.insert(cdg_source.clone());
        return build_and_store_media_g_pair(source, &cdg_source, library);
    }

    let metadata = metadata::read_from_path(source)?;
    let hash = sha256_for_file(source)?;
    let imported_at = current_unix_timestamp()?;

    let ext = source.extension().and_then(|e| e.to_str()).unwrap_or("bin");

    let dest = library.media_path(&hash, ext);
    if !dest.exists() {
        if let Some(parent) = dest.parent() {
            fs::create_dir_all(parent).with_context(|| {
                format!("failed to create media directory {}", parent.display())
            })?;
        }
        fs::copy(source, &dest).with_context(|| {
            format!("failed to copy {} to {}", source.display(), dest.display())
        })?;
    }

    let relative_path = format!("media/{}.{}", hash, ext);
    let title = metadata.title.or_else(|| {
        source
            .file_stem()
            .map(|stem| stem.to_string_lossy().into_owned())
    });

    Ok(Song {
        hash,
        file_path: Some(relative_path),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        language: None,
        audio_source_kind: "original".to_owned(),
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(ext.to_owned()),
    })
}

pub(super) fn build_and_store_media_g_pair(
    source: &Path,
    cdg_source: &Path,
    library: &LibraryRoot,
) -> Result<Song> {
    let metadata = metadata::read_from_path(source)?;
    let audio_bytes = fs::read(source)
        .with_context(|| format!("failed to read audio file at {}", source.display()))?;
    let cdg_bytes = fs::read(cdg_source)
        .with_context(|| format!("failed to read CDG file at {}", cdg_source.display()))?;
    // Media+G assets deliberately live under one shared convention so paired
    // files and MP3+G ZIPs behave the same way as they do in OpenKJ/Siglos libraries.
    let hash = media_g::media_g_hash(&audio_bytes, &cdg_bytes);
    let imported_at = current_unix_timestamp()?;
    let ext = source.extension().and_then(|e| e.to_str()).unwrap_or("bin");

    let audio_dest = library.media_g_audio_path(&hash, ext);
    copy_if_missing(source, &audio_dest)?;
    let cdg_dest = library.media_g_cdg_path(&hash);
    copy_if_missing(cdg_source, &cdg_dest)?;

    let title = metadata.title.or_else(|| {
        source
            .file_stem()
            .map(|stem| stem.to_string_lossy().into_owned())
    });

    Ok(Song {
        hash: hash.clone(),
        file_path: Some(format!("media-g/{}.{}", hash, ext)),
        cdg_path: Some(format!("media-g/{}.cdg", hash)),
        media_g_container: Some(MEDIA_G_PAIRED.to_owned()),
        instrumental: false,
        language: None,
        audio_source_kind: "original".to_owned(),
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(ext.to_owned()),
    })
}

pub(super) fn build_and_store_media_g_zip(source: &Path, library: &LibraryRoot) -> Result<Song> {
    let asset = media_g::inspect_zip_for_media_g(source)?;
    let metadata = metadata::read_from_bytes(&asset.audio_bytes, &asset.audio_extension)?;
    let hash = media_g::media_g_hash(&asset.audio_bytes, &asset.cdg_bytes);
    let imported_at = current_unix_timestamp()?;
    let dest = library.media_g_zip_path(&hash);
    copy_if_missing(source, &dest)?;

    let title = metadata.title.or_else(|| Some(asset.display_stem));

    Ok(Song {
        hash: hash.clone(),
        file_path: Some(format!("media-g/{}.zip", hash)),
        cdg_path: None,
        media_g_container: Some(MEDIA_G_ZIP.to_owned()),
        instrumental: false,
        language: None,
        audio_source_kind: "original".to_owned(),
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(asset.audio_extension),
    })
}

pub(super) fn copy_if_missing(source: &Path, destination: &Path) -> Result<()> {
    if destination.exists() {
        return Ok(());
    }

    if let Some(parent) = destination.parent() {
        fs::create_dir_all(parent)
            .with_context(|| format!("failed to create media directory {}", parent.display()))?;
    }
    fs::copy(source, destination).with_context(|| {
        format!(
            "failed to copy {} to {}",
            source.display(),
            destination.display()
        )
    })?;
    Ok(())
}

pub(super) fn sha256_for_file(path: &Path) -> Result<String> {
    let mut file = File::open(path)
        .with_context(|| format!("failed to open audio file at {}", path.display()))?;
    let mut hasher = Sha256::new();
    let mut buffer = [0_u8; 8 * 1024];

    loop {
        let bytes_read = file
            .read(&mut buffer)
            .with_context(|| format!("failed to read audio file at {}", path.display()))?;

        if bytes_read == 0 {
            break;
        }

        hasher.update(&buffer[..bytes_read]);
    }

    Ok(hash::hex_lower(hasher.finalize()))
}

pub(super) fn try_extract_embedded_lyrics(
    connection: &Connection,
    song: &Song,
    library: &LibraryRoot,
) {
    if let Ok(Some(_)) = cache::lyrics::get_lyrics_cache_entry(connection, &song.hash) {
        return;
    }

    let raw_lrc = match song.media_g_container.as_deref() {
        Some(MEDIA_G_ZIP) => {
            let archive_path = library.resolve(song.file_path.as_deref().unwrap());
            match media_g::inspect_zip_for_media_g(&archive_path).and_then(|asset| {
                read_embedded_lyrics_from_bytes(&asset.audio_bytes, &asset.audio_extension)
            }) {
                Ok(Some(lrc)) => lrc,
                _ => return,
            }
        }
        _ => {
            let resolved_path = library.resolve(song.file_path.as_deref().unwrap());
            match read_embedded_lyrics(&resolved_path) {
                Ok(Some(lrc)) => lrc,
                _ => return,
            }
        }
    };

    let fetched_at = current_unix_timestamp().unwrap_or(0);
    let entry = cache::lyrics::LyricsCacheEntry {
        song_hash: song.hash.clone(),
        lrc: raw_lrc,
        source: LyricsSource::Embedded,
        offset_ms: 0,
        fetched_at,
    };

    let _ = cache::lyrics::upsert_lyrics_cache_entry(connection, &entry);
}

pub(super) fn read_embedded_lyrics_from_bytes(
    bytes: &[u8],
    extension: &str,
) -> Result<Option<String>> {
    let reader = metadata::read_tagged_file_from_bytes(bytes, extension)
        .context("failed to inspect embedded lyrics in Media+G ZIP")?;

    for tag in reader.tags() {
        if let Some(lyrics) = tag.get_string(ItemKey::Lyrics) {
            let lyrics = lyrics.trim();
            if !lyrics.is_empty() {
                return Ok(Some(lyrics.to_owned()));
            }
        }
    }

    Ok(None)
}
