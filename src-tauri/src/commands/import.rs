use crate::{
    cache,
    commands::error::{database_error, internal_error, library_error, CommandResult},
    library::{ImportFailure, ImportSongsResult, Song},
    library_root::LibraryRoot,
    lyrics::fetch::{read_embedded_lyrics, LyricsSource},
    metadata, AppState,
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use serde::Serialize;
use sha2::{Digest, Sha256};
use std::{
    fs::{self, File},
    io::Read,
    path::Path,
    time::{SystemTime, UNIX_EPOCH},
};
use symphonia::core::{
    formats::FormatOptions,
    io::MediaSourceStream,
    meta::MetadataOptions,
    probe::Hint,
};
use tauri::State;

#[derive(Debug, Clone, Serialize)]
pub struct SongProperties {
    pub format: String,
    pub sample_rate: Option<u32>,
    pub channels: Option<u16>,
    pub bit_rate: Option<u32>,
    pub file_size: u64,
    pub duration_ms: i64,
    pub hash: String,
}

#[tauri::command]
pub fn import_songs(
    state: State<'_, AppState>,
    paths: Vec<String>,
) -> CommandResult<ImportSongsResult> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    Ok(import_songs_from_paths(&connection, &library, &paths))
}

#[tauri::command]
pub fn get_library(state: State<'_, AppState>) -> CommandResult<Vec<Song>> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    get_library_from_connection(&connection).map_err(|error| database_error(error.to_string()))
}

#[tauri::command]
pub fn search_library(state: State<'_, AppState>, query: String) -> CommandResult<Vec<Song>> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    cache::search_songs(&connection, &query).map_err(|error| database_error(error.to_string()))
}

pub fn import_songs_from_paths(
    connection: &Connection,
    library: &LibraryRoot,
    paths: &[String],
) -> ImportSongsResult {
    let mut imported = Vec::new();
    let mut failed = Vec::new();

    for path in paths {
        match build_and_store_song(Path::new(path), library) {
            Ok(song) => match cache::upsert_song(connection, &song) {
                Ok(()) => {
                    // Try to extract embedded lyrics from the audio file and
                    // store them so they're available without a network fetch.
                    try_extract_embedded_lyrics(connection, &song, library);
                    imported.push(song);
                }
                Err(error) => failed.push(ImportFailure {
                    path: path.clone(),
                    error: database_error(error.to_string()),
                }),
            },
            Err(error) => failed.push(ImportFailure {
                path: path.clone(),
                error: library_error(error.to_string()),
            }),
        }
    }

    ImportSongsResult { imported, failed }
}

pub fn get_library_from_connection(connection: &Connection) -> rusqlite::Result<Vec<Song>> {
    cache::list_songs(connection)
}

#[tauri::command]
pub fn update_song_metadata(
    state: State<'_, AppState>,
    hash: String,
    title: Option<String>,
    artist: Option<String>,
) -> CommandResult<Song> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    cache::update_song_title_artist(
        &connection,
        &hash,
        title.as_deref(),
        artist.as_deref(),
    )
    .map_err(|e| database_error(e.to_string()))?;

    cache::get_song_by_hash(&connection, &hash)
        .map_err(|e| database_error(e.to_string()))?
        .ok_or_else(|| database_error(format!("song with hash {hash} not found")))
}

#[tauri::command]
pub fn get_song_properties(
    state: State<'_, AppState>,
    song_id: String,
) -> CommandResult<SongProperties> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    let song = cache::get_song_by_hash(&connection, &song_id)
        .map_err(|e| database_error(e.to_string()))?
        .ok_or_else(|| database_error(format!("song with hash {song_id} not found")))?;

    let file_path = library.resolve(&song.file_path);

    let file_size = std::fs::metadata(&file_path)
        .map_err(|e| {
            library_error(format!(
                "failed to open audio file at {}: {}",
                file_path.display(),
                e
            ))
        })?
        .len();

    let ext = song
        .original_ext
        .as_deref()
        .or_else(|| {
            file_path
                .extension()
                .and_then(|e| e.to_str())
        })
        .unwrap_or("bin");

    let format = match ext.to_lowercase().as_str() {
        "mp3" => "MP3",
        "flac" => "FLAC",
        "wav" | "wave" => "WAV",
        "ogg" => "OGG",
        "aac" | "m4a" => "AAC/M4A",
        "opus" => "Opus",
        "aiff" | "aif" => "AIFF",
        other => other,
    }
    .to_owned();

    let file = File::open(&file_path).map_err(|e| {
        library_error(format!(
            "failed to open audio file at {}: {}",
            file_path.display(),
            e
        ))
    })?;
    let mss = MediaSourceStream::new(Box::new(file), Default::default());

    let mut hint = Hint::new();
    hint.with_extension(ext);

    let probed = symphonia::default::get_probe()
        .format(&hint, mss, &FormatOptions::default(), &MetadataOptions::default())
        .map_err(|e| {
            internal_error(format!(
                "failed to probe audio format for {}: {}",
                file_path.display(),
                e
            ))
        })?;

    let format_reader = probed.format;
    let (sample_rate, channels) = match format_reader.default_track() {
        Some(track) => {
            let cp = &track.codec_params;
            let sr = cp.sample_rate;
            let ch = cp.channels.map(|c| c.count() as u16);
            (sr, ch)
        }
        None => (None, None),
    };

    // Estimate bit rate from file size and duration when not available in
    // codec params (symphonia 0.5 does not expose a bit_rate field).
    let bit_rate = if song.duration_ms > 0 {
        let duration_secs = song.duration_ms as f64 / 1000.0;
        Some(((file_size as f64 * 8.0) / duration_secs / 1000.0).round() as u32)
    } else {
        None
    };

    Ok(SongProperties {
        format,
        sample_rate,
        channels,
        bit_rate,
        file_size,
        duration_ms: song.duration_ms,
        hash: song.hash,
    })
}

/// Hash the source file, copy it into the library's media directory, and build
/// a `Song` whose `file_path` is the *relative* path `media/{hash}.{ext}`.
fn build_and_store_song(source: &Path, library: &LibraryRoot) -> Result<Song> {
    let metadata = metadata::read_from_path(source)?;
    let hash = sha256_for_file(source)?;
    let imported_at = current_unix_timestamp()?;

    let ext = source
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("bin");

    // Copy the source file into the library media directory (skip if already
    // present, i.e. same content hash).
    let dest = library.media_path(&hash, ext);
    if !dest.exists() {
        if let Some(parent) = dest.parent() {
            fs::create_dir_all(parent)
                .with_context(|| format!("failed to create media directory {}", parent.display()))?;
        }
        fs::copy(source, &dest).with_context(|| {
            format!(
                "failed to copy {} to {}",
                source.display(),
                dest.display()
            )
        })?;
    }

    // Store the *relative* path so the library stays portable.
    let relative_path = format!("media/{}.{}", hash, ext);

    let title = metadata.title.or_else(|| {
        source
            .file_stem()
            .map(|stem| stem.to_string_lossy().into_owned())
    });

    Ok(Song {
        hash,
        file_path: relative_path,
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(ext.to_owned()),
    })
}

fn sha256_for_file(path: &Path) -> Result<String> {
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

    Ok(format!("{:x}", hasher.finalize()))
}

fn current_unix_timestamp() -> Result<i64> {
    let duration = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .context("system clock is set before Unix epoch")?;

    Ok(duration.as_secs() as i64)
}

/// Attempt to read embedded lyrics from the audio file and persist them into
/// the lyrics cache. This is best-effort: failures are silently ignored so they
/// never block a successful song import.
fn try_extract_embedded_lyrics(
    connection: &Connection,
    song: &Song,
    library: &LibraryRoot,
) {
    // Don't overwrite lyrics that are already cached (e.g. from a previous
    // import or manual entry).
    if let Ok(Some(_)) = cache::lyrics::get_lyrics_cache_entry(connection, &song.hash) {
        return;
    }

    let resolved_path = library.resolve(&song.file_path);
    let raw_lrc = match read_embedded_lyrics(&resolved_path) {
        Ok(Some(lrc)) => lrc,
        _ => return,
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
