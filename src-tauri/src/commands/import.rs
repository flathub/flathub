use crate::{
    cache,
    commands::error::{database_error, library_error, CommandResult},
    library::{ImportFailure, ImportSongsResult, Song},
    library_root::LibraryRoot,
    metadata, AppState,
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use sha2::{Digest, Sha256};
use std::{
    fs::{self, File},
    io::Read,
    path::Path,
    time::{SystemTime, UNIX_EPOCH},
};
use tauri::State;

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
                Ok(()) => imported.push(song),
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
