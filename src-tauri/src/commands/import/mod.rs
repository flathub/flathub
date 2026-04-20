mod cover;
mod delete;
mod expand;
mod ingest;
mod preview;
mod types;

pub use cover::extract_embedded_cover_art_from_connection;
pub use types::{
    DeleteSongsFailure, DeleteSongsResult, ExpandedImportPaths, ExtractEmbeddedCoverArtFailure,
    ExtractEmbeddedCoverArtResult, ImportCandidateDetails, ImportSongsOptions, SongProperties,
};

use crate::{
    audio::decode,
    cache,
    commands::error::{
        database_error, internal_error, library_error, state_lock_error, CommandResult,
    },
    library::{ImportFailure, ImportSongsResult, Song},
    library_root::LibraryRoot,
    media_g::{self, MEDIA_G_PAIRED, MEDIA_G_ZIP},
    AppState,
};
use rusqlite::Connection;
use std::collections::HashSet;
use tauri::State;

use delete::delete_song_from_library;
use expand::{build_selected_cdg_lookup, classify_import_paths, collect_expandable_import_paths};
use ingest::{build_and_store_media_g_zip, build_and_store_song, try_extract_embedded_lyrics};
use preview::{display_audio_format, inspect_import_candidate};

#[cfg(target_os = "macos")]
use std::ffi::{c_char, CStr, CString};

#[tauri::command]
pub fn import_songs(
    state: State<'_, AppState>,
    paths: Vec<String>,
    options: Option<ImportSongsOptions>,
) -> CommandResult<ImportSongsResult> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    Ok(import_songs_from_paths_with_options(
        &connection,
        &library,
        &paths,
        &options.unwrap_or_default(),
    ))
}

#[tauri::command]
pub fn get_import_candidate_details(
    paths: Vec<String>,
) -> CommandResult<Vec<ImportCandidateDetails>> {
    paths
        .into_iter()
        .map(|raw_path| {
            inspect_import_candidate(&raw_path).map_err(|error| library_error(error.to_string()))
        })
        .collect()
}

#[tauri::command]
pub fn expand_import_paths(paths: Vec<String>) -> CommandResult<ExpandedImportPaths> {
    Ok(collect_expandable_import_paths(&paths))
}

#[cfg(target_os = "macos")]
unsafe extern "C" {
    fn openkara_pick_import_paths(
        default_path: *const c_char,
        count_out: *mut usize,
    ) -> *mut *mut c_char;
    fn openkara_free_import_paths(paths: *mut *mut c_char, count: usize);
}

#[tauri::command]
pub fn pick_import_paths(default_path: Option<String>) -> CommandResult<Vec<String>> {
    #[cfg(target_os = "macos")]
    {
        let default_path = default_path
            .as_deref()
            .map(CString::new)
            .transpose()
            .map_err(|error| library_error(error.to_string()))?;
        let mut count = 0usize;
        let raw_paths = unsafe {
            openkara_pick_import_paths(
                default_path
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value.as_ptr()),
                &mut count,
            )
        };

        if raw_paths.is_null() || count == 0 {
            return Ok(Vec::new());
        }

        let mut collected_paths = Vec::with_capacity(count);
        for index in 0..count {
            let raw_path = unsafe { *raw_paths.add(index) };
            if raw_path.is_null() {
                continue;
            }

            let path = unsafe { CStr::from_ptr(raw_path) }
                .to_string_lossy()
                .into_owned();
            collected_paths.push(path);
        }

        unsafe {
            openkara_free_import_paths(raw_paths, count);
        }

        return Ok(collected_paths);
    }

    #[cfg(not(target_os = "macos"))]
    {
        let _ = default_path;
        Err(library_error(
            "mixed file and folder selection is only available on macOS".to_string(),
        ))
    }
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

#[tauri::command]
pub fn delete_songs(
    state: State<'_, AppState>,
    song_ids: Vec<String>,
) -> CommandResult<DeleteSongsResult> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;
    let current_song_id = {
        let playback = state
            .playback
            .lock()
            .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
        playback.current_song_id().map(|value| value.to_owned())
    };

    let mut deleted_song_ids = Vec::new();
    let mut failed = Vec::new();

    for song_id in song_ids {
        match delete_song_from_library(&connection, &library, &song_id) {
            Ok(()) => deleted_song_ids.push(song_id),
            Err(error) => failed.push(DeleteSongsFailure {
                song_id,
                error: library_error(error.to_string()),
            }),
        }
    }

    if current_song_id
        .as_deref()
        .is_some_and(|song_id| deleted_song_ids.iter().any(|deleted| deleted == song_id))
    {
        {
            let mut playback = state
                .playback
                .lock()
                .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
            playback.clear_track();
        }
        let mut cdg_state = state
            .cdg_state
            .lock()
            .map_err(|_| state_lock_error("CDG state lock was poisoned"))?;
        *cdg_state = None;
    }

    Ok(DeleteSongsResult {
        deleted_song_ids,
        failed,
    })
}

#[tauri::command]
pub fn extract_embedded_cover_art(
    state: State<'_, AppState>,
    song_ids: Vec<String>,
) -> CommandResult<ExtractEmbeddedCoverArtResult> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    Ok(extract_embedded_cover_art_from_connection(
        &connection,
        &library,
        &song_ids,
    ))
}

pub fn import_songs_from_paths(
    connection: &Connection,
    library: &LibraryRoot,
    paths: &[String],
) -> ImportSongsResult {
    import_songs_from_paths_with_options(connection, library, paths, &ImportSongsOptions::default())
}

pub fn import_songs_from_paths_with_options(
    connection: &Connection,
    library: &LibraryRoot,
    paths: &[String],
    options: &ImportSongsOptions,
) -> ImportSongsResult {
    let mut imported = Vec::new();
    let mut failed = Vec::new();
    let classified = classify_import_paths(paths);
    let selected_cdg_by_stem = build_selected_cdg_lookup(&classified.cdg_paths);
    let mut consumed_cdg_paths = HashSet::new();

    for audio_path in &classified.audio_paths {
        match build_and_store_song(
            audio_path,
            library,
            &selected_cdg_by_stem,
            &options.explicit_cdg_by_audio_path,
            &options.skip_cdg_for_audio_paths,
            &mut consumed_cdg_paths,
        ) {
            Ok(song) => match cache::upsert_song(connection, &song) {
                Ok(()) => {
                    try_extract_embedded_lyrics(connection, &song, library);
                    imported.push(song);
                }
                Err(error) => failed.push(ImportFailure {
                    path: audio_path.display().to_string(),
                    error: database_error(error.to_string()),
                }),
            },
            Err(error) => failed.push(ImportFailure {
                path: audio_path.display().to_string(),
                error: library_error(error.to_string()),
            }),
        }
    }

    for zip_path in &classified.zip_paths {
        match build_and_store_media_g_zip(zip_path, library) {
            Ok(song) => match cache::upsert_song(connection, &song) {
                Ok(()) => imported.push(song),
                Err(error) => failed.push(ImportFailure {
                    path: zip_path.display().to_string(),
                    error: database_error(error.to_string()),
                }),
            },
            Err(error) => failed.push(ImportFailure {
                path: zip_path.display().to_string(),
                error: library_error(error.to_string()),
            }),
        }
    }

    for cdg_path in &classified.cdg_paths {
        if consumed_cdg_paths.contains(cdg_path) {
            continue;
        }

        failed.push(ImportFailure {
            path: cdg_path.display().to_string(),
            error: library_error(format!(
                "standalone .cdg file {} does not have a matching audio track",
                cdg_path.display()
            )),
        });
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

    cache::update_song_title_artist(&connection, &hash, title.as_deref(), artist.as_deref())
        .map_err(|e| database_error(e.to_string()))?;

    cache::get_song_by_hash(&connection, &hash)
        .map_err(|e| database_error(e.to_string()))?
        .ok_or_else(|| database_error(format!("song with hash {hash} not found")))
}

#[tauri::command]
pub fn set_songs_instrumental(
    state: State<'_, AppState>,
    song_ids: Vec<String>,
    instrumental: bool,
) -> CommandResult<Vec<Song>> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    set_songs_instrumental_in_connection(&connection, &song_ids, instrumental)
}

pub fn set_songs_instrumental_in_connection(
    connection: &Connection,
    song_ids: &[String],
    instrumental: bool,
) -> CommandResult<Vec<Song>> {
    let mut updated_songs = Vec::with_capacity(song_ids.len());

    for song_id in song_ids {
        let updated = cache::update_song_instrumental(connection, song_id, instrumental)
            .map_err(|error| database_error(error.to_string()))?;
        if updated == 0 {
            return Err(database_error(format!(
                "song with hash {song_id} not found"
            )));
        }

        let song = cache::get_song_by_hash(connection, song_id)
            .map_err(|error| database_error(error.to_string()))?
            .ok_or_else(|| database_error(format!("song with hash {song_id} not found")))?;
        updated_songs.push(song);
    }

    Ok(updated_songs)
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

    let Some(song_path) = song.file_path.as_deref() else {
        return Err(library_error(format!(
            "song {} does not have a local file path",
            song_id
        )));
    };
    let file_path = library.resolve(song_path);
    let ext = song
        .original_ext
        .as_deref()
        .or_else(|| file_path.extension().and_then(|e| e.to_str()))
        .unwrap_or("bin");

    let (decoded, file_size, format) = if song.media_g_container.as_deref() == Some(MEDIA_G_ZIP) {
        let asset = media_g::inspect_zip_for_media_g(&file_path)
            .map_err(|error| library_error(error.to_string()))?;
        let decoded = decode::decode_bytes(asset.audio_bytes, ext).map_err(|e| {
            internal_error(format!("failed to decode audio for {}: {}", song_id, e))
        })?;
        let file_size = std::fs::metadata(&file_path)
            .map_err(|e| {
                library_error(format!(
                    "failed to open Media+G ZIP at {}: {}",
                    file_path.display(),
                    e
                ))
            })?
            .len();
        (
            decoded,
            file_size,
            format!("{}+G ZIP", display_audio_format(ext)),
        )
    } else {
        let decoded = decode::decode_file(&file_path).map_err(|e| {
            internal_error(format!(
                "failed to decode audio for {}: {}",
                file_path.display(),
                e
            ))
        })?;
        let file_size = std::fs::metadata(&file_path)
            .map_err(|e| {
                library_error(format!(
                    "failed to open audio file at {}: {}",
                    file_path.display(),
                    e
                ))
            })?
            .len();
        let format = if song.media_g_container.as_deref() == Some(MEDIA_G_PAIRED) {
            format!("{}+G", display_audio_format(ext))
        } else {
            display_audio_format(ext).to_owned()
        };
        (decoded, file_size, format)
    };

    let bit_rate = if song.duration_ms > 0 {
        let duration_secs = song.duration_ms as f64 / 1000.0;
        Some(((file_size as f64 * 8.0) / duration_secs / 1000.0).round() as u32)
    } else {
        None
    };

    Ok(SongProperties {
        format,
        sample_rate: Some(decoded.sample_rate),
        channels: Some(decoded.channels as u16),
        bit_rate,
        file_size,
        duration_ms: song.duration_ms,
        hash: song.hash,
    })
}

#[cfg(test)]
mod tests {
    use super::expand::collect_expandable_import_paths;
    use std::fs;
    use tempfile::TempDir;

    fn write_file(path: &std::path::Path) {
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).expect("failed to create test directory");
        }
        fs::write(path, b"test").expect("failed to write test file");
    }

    #[test]
    fn expands_importable_files_recursively() {
        let tempdir = TempDir::new().expect("failed to create tempdir");
        let root = tempdir.path();

        write_file(&root.join("one.mp3"));
        write_file(&root.join("two.lrc"));
        write_file(&root.join("three.cdg"));
        write_file(&root.join("nested/four.zip"));
        write_file(&root.join("nested/deeper/five.flac"));
        write_file(&root.join("nested/deeper/too/deep/hidden.mp3"));

        let result = collect_expandable_import_paths(&[root.display().to_string()]);

        assert_eq!(result.song_count, 3);
        assert!(result.paths.iter().any(|path| path.ends_with("one.mp3")));
        assert!(result.paths.iter().any(|path| path.ends_with("two.lrc")));
        assert!(result.paths.iter().any(|path| path.ends_with("three.cdg")));
        assert!(result
            .paths
            .iter()
            .any(|path| path.ends_with("nested/four.zip")));
        assert!(result
            .paths
            .iter()
            .any(|path| path.ends_with("nested/deeper/five.flac")));
        assert!(!result
            .paths
            .iter()
            .any(|path| path.ends_with("nested/deeper/too/deep/hidden.mp3")));
    }

    #[test]
    fn caps_recursive_import_scanning_depth() {
        let tempdir = TempDir::new().expect("failed to create tempdir");
        let root = tempdir.path();

        write_file(&root.join("level-0.mp3"));
        write_file(&root.join("level-1/level-1.mp3"));
        write_file(&root.join("level-1/level-2/level-2.mp3"));
        write_file(&root.join("level-1/level-2/level-3/level-3.mp3"));
        write_file(&root.join("level-1/level-2/level-3/level-4/level-5/level-5.mp3"));

        let result = collect_expandable_import_paths(&[root.join("level-1").display().to_string()]);

        assert_eq!(result.song_count, 3);
        assert!(result
            .paths
            .iter()
            .any(|path| path.ends_with("level-1/level-1.mp3")));
        assert!(result
            .paths
            .iter()
            .any(|path| path.ends_with("level-1/level-2/level-2.mp3")));
        assert!(result
            .paths
            .iter()
            .any(|path| path.ends_with("level-1/level-2/level-3/level-3.mp3")));
        assert!(!result
            .paths
            .iter()
            .any(|path| path.ends_with("level-1/level-2/level-3/level-4/level-5/level-5.mp3")));
    }
}
