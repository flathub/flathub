use crate::{
    cache,
    cache::lyrics::LyricsCacheEntry,
    commands::error::{database_error, lyrics_error, CommandResult},
    commands::remote_library,
    library::Song,
    library_root::LibraryRoot,
    lyrics::{
        self,
        fetch::{
            fetch_online_timed_lyrics, lookup_query_from_song, LyricsSource, TimedLyricsProvider,
        },
        lrcapi::LrcApiClient,
        lrclib::LrcLibClient,
        parser::LyricLine,
    },
    AppState,
};
use anyhow::{bail, Context, Result};
use rusqlite::Connection;
use serde::Serialize;
use std::path::Path;
use tauri::{AppHandle, State};

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct LyricsPayload {
    pub song_id: String,
    pub lines: Vec<LyricLine>,
    pub source: Option<LyricsSource>,
    pub offset_ms: i64,
    pub raw_lrc: String,
}

#[tauri::command]
pub fn fetch_lyrics(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<LyricsPayload> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    let lrclib_client = LrcLibClient::new_default();
    let lrcapi_client = LrcApiClient::new_default();

    fetch_lyrics_from_connection(
        &connection,
        &library_root,
        &lrclib_client,
        &lrcapi_client,
        &song_id,
    )
    .and_then(|payload| {
        remote_library::sync_active_remote_database_if_needed(&state.app_data_dir)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
        remote_library::maybe_publish_song_to_bound_remote(&state, &app_handle, &song_id)
            .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
        Ok(payload)
    })
    .map_err(|error| {
        // Lower-level lyrics modules still return anyhow errors. Classify them
        // here so UI-facing commands expose stable error codes and fallback hints
        // before the internal modules are fully migrated to typed domain errors.
        lyrics_error(error.to_string())
    })
}

#[tauri::command]
pub fn set_lyrics_offset(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
    ms: i64,
) -> CommandResult<()> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    set_lyrics_offset_in_connection(&connection, &song_id, ms)
        .map_err(|error| lyrics_error(error.to_string()))?;
    remote_library::sync_active_remote_database_if_needed(&state.app_data_dir)?;
    remote_library::maybe_publish_song_to_bound_remote(&state, &app_handle, &song_id)?;
    Ok(())
}

pub fn fetch_lyrics_from_connection(
    connection: &Connection,
    library_root: &LibraryRoot,
    lrclib_client: &LrcLibClient,
    lrcapi_client: &LrcApiClient,
    song_id: &str,
) -> Result<LyricsPayload> {
    let song = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;

    // Lyrics are cached by the stable song hash so repeat fetches can skip both
    // network and filesystem fallbacks once a synced source has been resolved.
    if let Some(cached) = cache::lyrics::get_lyrics_cache_entry(connection, song_id)? {
        return payload_from_cached_entry(song.hash, cached);
    }

    let Some(song_path) = song.file_path.as_deref() else {
        return Ok(LyricsPayload {
            song_id: song.hash,
            lines: Vec::new(),
            source: None,
            offset_ms: 0,
            raw_lrc: String::new(),
        });
    };
    let resolved_path = library_root.resolve(song_path);
    let providers = [
        TimedLyricsProvider::LrcLib(lrclib_client),
        TimedLyricsProvider::LrcApi(lrcapi_client),
    ];

    // Online requests are opportunistic: if they fail, we still want embedded
    // and sidecar sources to rescue the fetch instead of failing the whole song.
    let Some(fetched) = lyrics::fetch::fetch_lyrics_for_song(&providers, &song, &resolved_path)?
    else {
        return Ok(LyricsPayload {
            song_id: song.hash,
            lines: Vec::new(),
            source: None,
            offset_ms: 0,
            raw_lrc: String::new(),
        });
    };

    let lines = lyrics::parser::parse_lrc(&fetched.raw_lrc)
        .with_context(|| format!("failed to parse synced lyrics for song {song_id}"))?;
    let source = fetched.source;
    let raw_lrc = fetched.raw_lrc.clone();
    cache::lyrics::upsert_lyrics_cache_entry(
        connection,
        &LyricsCacheEntry {
            song_hash: song.hash.clone(),
            lrc: fetched.raw_lrc,
            source: source.clone(),
            offset_ms: 0,
            fetched_at: current_unix_timestamp()?,
        },
    )
    .context("failed to cache fetched lyrics")?;

    Ok(LyricsPayload {
        song_id: song.hash,
        lines,
        source: Some(source),
        offset_ms: 0,
        raw_lrc,
    })
}

pub fn set_lyrics_offset_in_connection(
    connection: &Connection,
    song_id: &str,
    ms: i64,
) -> Result<()> {
    let song_exists = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .is_some();
    if !song_exists {
        bail!("song with hash {song_id} was not found in the library");
    }

    if cache::lyrics::get_lyrics_cache_entry(connection, song_id)?.is_none() {
        bail!("song with hash {song_id} does not have cached lyrics");
    }

    cache::lyrics::set_lyrics_offset(connection, song_id, ms)
        .context("failed to persist lyrics offset")?;

    Ok(())
}

fn payload_from_cached_entry(song_id: String, cached: LyricsCacheEntry) -> Result<LyricsPayload> {
    let mut lines = lyrics::parser::parse_lrc(&cached.lrc)
        .with_context(|| format!("failed to parse cached synced lyrics for song {song_id}"))?;

    if lines.is_empty() {
        lines = plain_text_to_lines(&cached.lrc);
    }

    Ok(LyricsPayload {
        song_id,
        lines,
        source: Some(cached.source),
        offset_ms: cached.offset_ms,
        raw_lrc: cached.lrc,
    })
}

#[tauri::command]
pub fn save_manual_lyrics(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
    text: String,
) -> CommandResult<LyricsPayload> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    // Try parsing as LRC first
    let lines = match lyrics::parser::parse_lrc(&text) {
        Ok(parsed) if !parsed.is_empty() => parsed,
        _ => plain_text_to_lines(&text),
    };

    let raw_lrc = text.clone();

    let fetched_at = current_unix_timestamp().map_err(|e| lyrics_error(e.to_string()))?;

    cache::lyrics::upsert_lyrics_cache_entry(
        &connection,
        &LyricsCacheEntry {
            song_hash: song_id.clone(),
            lrc: text,
            source: LyricsSource::Manual,
            offset_ms: 0,
            fetched_at,
        },
    )
    .map_err(|e| database_error(e.to_string()))?;

    remote_library::sync_active_remote_database_if_needed(&state.app_data_dir)?;
    remote_library::maybe_publish_song_to_bound_remote(&state, &app_handle, &song_id)?;

    Ok(LyricsPayload {
        song_id,
        lines,
        source: Some(LyricsSource::Manual),
        offset_ms: 0,
        raw_lrc,
    })
}

#[derive(Debug, Clone, Serialize)]
pub struct LyricsMatch {
    pub song_id: String,
    pub lrc_path: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct ImportLyricsResult {
    pub matched: Vec<LyricsMatch>,
    pub unmatched: Vec<String>,
}

#[tauri::command]
pub fn import_lyrics_files(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    paths: Vec<String>,
) -> CommandResult<ImportLyricsResult> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    let all_songs = cache::list_songs(&connection).map_err(|e| database_error(e.to_string()))?;

    let mut matched = Vec::new();
    let mut unmatched = Vec::new();

    for path_str in &paths {
        let path = Path::new(path_str);

        // Read LRC file content
        let content = match std::fs::read_to_string(path) {
            Ok(c) => c,
            Err(_) => {
                unmatched.push(path_str.clone());
                continue;
            }
        };

        // Try filename matching first
        let lrc_stem = path
            .file_stem()
            .and_then(|s| s.to_str())
            .map(|s| s.to_lowercase());

        let mut found_song: Option<&Song> = None;

        if let Some(ref stem) = lrc_stem {
            found_song = all_songs.iter().find(|song| {
                let Some(song_path) = song.file_path.as_deref() else {
                    return false;
                };
                let song_path = Path::new(song_path);
                let song_stem = song_path
                    .file_stem()
                    .and_then(|s| s.to_str())
                    .map(|s| s.to_lowercase());
                song_stem.as_deref() == Some(stem.as_str())
            });
        }

        // If no filename match, try metadata matching
        if found_song.is_none() {
            let meta = lyrics::parser::parse_lrc_metadata(&content);
            if let (Some(ref lrc_artist), Some(ref lrc_title)) = (meta.artist, meta.title) {
                let artist_lower = lrc_artist.to_lowercase();
                let title_lower = lrc_title.to_lowercase();
                found_song = all_songs.iter().find(|song| {
                    let song_artist = song.artist.as_deref().unwrap_or("").to_lowercase();
                    let song_title = song.title.as_deref().unwrap_or("").to_lowercase();
                    song_artist == artist_lower && song_title == title_lower
                });
            }
        }

        if let Some(song) = found_song {
            let offset_ms = lyrics::parser::parse_lrc_metadata(&content)
                .offset_ms
                .unwrap_or(0);

            let fetched_at = current_unix_timestamp().map_err(|e| lyrics_error(e.to_string()))?;

            let entry = LyricsCacheEntry {
                song_hash: song.hash.clone(),
                lrc: content,
                source: LyricsSource::Manual,
                offset_ms,
                fetched_at,
            };

            let _ = cache::lyrics::upsert_lyrics_cache_entry(&connection, &entry);

            matched.push(LyricsMatch {
                song_id: song.hash.clone(),
                lrc_path: path_str.clone(),
            });
        } else {
            unmatched.push(path_str.clone());
        }
    }

    if !matched.is_empty() {
        remote_library::sync_active_remote_database_if_needed(&state.app_data_dir)?;
        let matched_song_ids: Vec<String> =
            matched.iter().map(|entry| entry.song_id.clone()).collect();
        remote_library::maybe_publish_songs_to_bound_remote(
            &state,
            &app_handle,
            &matched_song_ids,
        )?;
    }

    Ok(ImportLyricsResult { matched, unmatched })
}

#[tauri::command]
pub fn extract_embedded_lyrics(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<LyricsPayload> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;

    let song = cache::get_song_by_hash(&connection, &song_id)
        .map_err(|e| lyrics_error(e.to_string()))?
        .ok_or_else(|| lyrics_error(format!("song {song_id} not found")))?;

    let Some(song_path) = song.file_path.as_deref() else {
        return Err(lyrics_error(format!(
            "song {} does not have a local file path",
            song_id
        )));
    };
    let resolved_path = library_root.resolve(song_path);
    let embedded = lyrics::fetch::read_embedded_lyrics(&resolved_path)
        .map_err(|e| lyrics_error(e.to_string()))?
        .ok_or_else(|| lyrics_error("No embedded lyrics found in this file".to_owned()))?;

    // Parse and cache
    let lines = match lyrics::parser::parse_lrc(&embedded) {
        Ok(parsed) if !parsed.is_empty() => parsed,
        _ => plain_text_to_lines(&embedded),
    };

    let raw_lrc = embedded.clone();

    let fetched_at = current_unix_timestamp().map_err(|e| lyrics_error(e.to_string()))?;

    cache::lyrics::upsert_lyrics_cache_entry(
        &connection,
        &LyricsCacheEntry {
            song_hash: song_id.clone(),
            lrc: embedded,
            source: LyricsSource::Embedded,
            offset_ms: 0,
            fetched_at,
        },
    )
    .map_err(|e| database_error(e.to_string()))?;

    remote_library::sync_active_remote_database_if_needed(&state.app_data_dir)?;
    remote_library::maybe_publish_song_to_bound_remote(&state, &app_handle, &song_id)?;

    Ok(LyricsPayload {
        song_id,
        lines,
        source: Some(LyricsSource::Embedded),
        offset_ms: 0,
        raw_lrc,
    })
}

#[tauri::command]
pub fn fetch_lyrics_online(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<LyricsPayload> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;

    let song = cache::get_song_by_hash(&connection, &song_id)
        .map_err(|e| lyrics_error(e.to_string()))?
        .ok_or_else(|| lyrics_error(format!("song {song_id} not found")))?;

    let lrclib_client = LrcLibClient::new_default();
    let lrcapi_client = LrcApiClient::new_default();
    let providers = [
        TimedLyricsProvider::LrcLib(&lrclib_client),
        TimedLyricsProvider::LrcApi(&lrcapi_client),
    ];
    let query = match lookup_query_from_song(&song) {
        Some(q) => q,
        None => {
            return Ok(LyricsPayload {
                song_id: song.hash,
                lines: Vec::new(),
                source: None,
                offset_ms: 0,
                raw_lrc: String::new(),
            });
        }
    };

    let Some(fetched) =
        fetch_online_timed_lyrics(&providers, &query).map_err(|e| lyrics_error(e.to_string()))?
    else {
        return Ok(LyricsPayload {
            song_id: song.hash,
            lines: Vec::new(),
            source: None,
            offset_ms: 0,
            raw_lrc: String::new(),
        });
    };

    let lines =
        lyrics::parser::parse_lrc(&fetched.raw_lrc).map_err(|e| lyrics_error(e.to_string()))?;

    let fetched_at = current_unix_timestamp().map_err(|e| lyrics_error(e.to_string()))?;

    cache::lyrics::upsert_lyrics_cache_entry(
        &connection,
        &LyricsCacheEntry {
            song_hash: song_id.clone(),
            lrc: fetched.raw_lrc.clone(),
            source: fetched.source.clone(),
            offset_ms: 0,
            fetched_at,
        },
    )
    .map_err(|e| database_error(e.to_string()))?;

    remote_library::sync_active_remote_database_if_needed(&state.app_data_dir)?;
    remote_library::maybe_publish_song_to_bound_remote(&state, &app_handle, &song_id)?;

    Ok(LyricsPayload {
        song_id,
        lines,
        source: Some(fetched.source),
        offset_ms: 0,
        raw_lrc: fetched.raw_lrc,
    })
}

/// Convert plain text (no LRC timestamps) into `LyricLine` entries with
/// `time_ms: 0` so the frontend can display them as unsynced lyrics.
fn plain_text_to_lines(text: &str) -> Vec<LyricLine> {
    text.lines()
        .filter(|l| !l.trim().is_empty())
        .map(|l| LyricLine {
            time_ms: 0,
            text: l.to_string(),
            words: None,
        })
        .collect()
}

use super::error::current_unix_timestamp;
