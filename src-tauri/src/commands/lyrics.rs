use crate::{
    cache,
    cache::lyrics::LyricsCacheEntry,
    commands::error::{database_error, lyrics_error, CommandResult},
    library::Song,
    library_root::LibraryRoot,
    lyrics::{self, fetch::LyricsSource, lrclib::LrcLibClient, parser::LyricLine},
    AppState,
};
use anyhow::{bail, Context, Result};
use rusqlite::Connection;
use serde::Serialize;
use std::path::Path;
use std::time::{SystemTime, UNIX_EPOCH};
use tauri::State;

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct LyricsPayload {
    pub song_id: String,
    pub lines: Vec<LyricLine>,
    pub source: Option<LyricsSource>,
    pub offset_ms: i64,
}

#[tauri::command]
pub fn fetch_lyrics(state: State<'_, AppState>, song_id: String) -> CommandResult<LyricsPayload> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;

    fetch_lyrics_from_connection(&connection, &library_root, &LrcLibClient::new_default(), &song_id).map_err(
        |error| {
            // Lower-level lyrics modules still return anyhow errors. Classify them
            // here so UI-facing commands expose stable error codes and fallback hints
            // before the internal modules are fully migrated to typed domain errors.
            lyrics_error(error.to_string())
        },
    )
}

#[tauri::command]
pub fn set_lyrics_offset(
    state: State<'_, AppState>,
    song_id: String,
    ms: i64,
) -> CommandResult<()> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    set_lyrics_offset_in_connection(&connection, &song_id, ms)
        .map_err(|error| lyrics_error(error.to_string()))
}

pub fn fetch_lyrics_from_connection(
    connection: &Connection,
    library_root: &LibraryRoot,
    client: &LrcLibClient,
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

    let resolved_path = library_root.resolve(&song.file_path);
    let Some(fetched) = lyrics::fetch::fetch_lyrics_for_song(client, &song, &resolved_path)? else {
        return Ok(LyricsPayload {
            song_id: song.hash,
            lines: Vec::new(),
            source: None,
            offset_ms: 0,
        });
    };

    let lines = lyrics::parser::parse_lrc(&fetched.raw_lrc)
        .with_context(|| format!("failed to parse synced lyrics for song {song_id}"))?;
    let source = fetched.source;
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
    let lines = lyrics::parser::parse_lrc(&cached.lrc)
        .with_context(|| format!("failed to parse cached synced lyrics for song {song_id}"))?;

    Ok(LyricsPayload {
        song_id,
        lines,
        source: Some(cached.source),
        offset_ms: cached.offset_ms,
    })
}

#[tauri::command]
pub fn save_manual_lyrics(
    state: State<'_, AppState>,
    song_id: String,
    text: String,
) -> CommandResult<LyricsPayload> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    // Try parsing as LRC first
    let lines = match lyrics::parser::parse_lrc(&text) {
        Ok(parsed) if !parsed.is_empty() => parsed,
        _ => Vec::new(), // plain text — store raw but return empty lines (frontend handles display)
    };

    let fetched_at = current_unix_timestamp()
        .map_err(|e| lyrics_error(e.to_string()))?;

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

    Ok(LyricsPayload {
        song_id,
        lines,
        source: Some(LyricsSource::Manual),
        offset_ms: 0,
    })
}

#[derive(Debug, Clone, Serialize)]
pub struct LyricsMatch {
    pub song_hash: String,
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
    paths: Vec<String>,
) -> CommandResult<ImportLyricsResult> {
    let library = state.library_root()?;
    let connection = cache::open_database(&library.database_path()).map_err(database_error)?;

    let all_songs = cache::list_songs(&connection)
        .map_err(|e| database_error(e.to_string()))?;

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
        let lrc_stem = path.file_stem()
            .and_then(|s| s.to_str())
            .map(|s| s.to_lowercase());

        let mut found_song: Option<&Song> = None;

        if let Some(ref stem) = lrc_stem {
            found_song = all_songs.iter().find(|song| {
                let song_path = Path::new(&song.file_path);
                let song_stem = song_path.file_stem()
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

            let fetched_at = current_unix_timestamp()
                .map_err(|e| lyrics_error(e.to_string()))?;

            let entry = LyricsCacheEntry {
                song_hash: song.hash.clone(),
                lrc: content,
                source: LyricsSource::Manual,
                offset_ms,
                fetched_at,
            };

            let _ = cache::lyrics::upsert_lyrics_cache_entry(&connection, &entry);

            matched.push(LyricsMatch {
                song_hash: song.hash.clone(),
                lrc_path: path_str.clone(),
            });
        } else {
            unmatched.push(path_str.clone());
        }
    }

    Ok(ImportLyricsResult { matched, unmatched })
}

fn current_unix_timestamp() -> Result<i64> {
    let duration = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .context("system clock is set before Unix epoch")?;

    Ok(duration.as_secs() as i64)
}
