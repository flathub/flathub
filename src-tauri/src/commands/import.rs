use crate::{
    audio::decode,
    cache,
    commands::error::{
        database_error, internal_error, library_error, state_lock_error, CommandError,
        CommandResult,
    },
    library::{ImportFailure, ImportSongsResult, Song},
    library_root::LibraryRoot,
    lyrics::fetch::{read_embedded_lyrics, LyricsSource},
    media_g::{self, MEDIA_G_PAIRED, MEDIA_G_ZIP},
    metadata, AppState,
};
use anyhow::{Context, Result};
use lofty::{file::TaggedFileExt, tag::ItemKey};
use rusqlite::{params, Connection};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::{HashMap, HashSet},
    fs::{self, File},
    io::Read,
    path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
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

#[derive(Debug, Clone, Serialize)]
pub struct DeleteSongsFailure {
    pub song_id: String,
    pub error: CommandError,
}

#[derive(Debug, Clone, Serialize)]
pub struct DeleteSongsResult {
    pub deleted_song_ids: Vec<String>,
    pub failed: Vec<DeleteSongsFailure>,
}

#[derive(Debug, Clone, Default, Deserialize, Serialize)]
pub struct ImportSongsOptions {
    #[serde(default)]
    pub explicit_cdg_by_audio_path: HashMap<String, String>,
    #[serde(default)]
    pub skip_cdg_for_audio_paths: Vec<String>,
}

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

fn build_and_store_song(
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
        file_path: relative_path,
        cdg_path: None,
        media_g_container: None,
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(ext.to_owned()),
    })
}

fn build_and_store_media_g_pair(
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
        file_path: format!("media-g/{}.{}", hash, ext),
        cdg_path: Some(format!("media-g/{}.cdg", hash)),
        media_g_container: Some(MEDIA_G_PAIRED.to_owned()),
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(ext.to_owned()),
    })
}

fn build_and_store_media_g_zip(source: &Path, library: &LibraryRoot) -> Result<Song> {
    let asset = media_g::inspect_zip_for_media_g(source)?;
    let metadata = metadata::read_from_bytes(&asset.audio_bytes, &asset.audio_extension)?;
    let hash = media_g::media_g_hash(&asset.audio_bytes, &asset.cdg_bytes);
    let imported_at = current_unix_timestamp()?;
    let dest = library.media_g_zip_path(&hash);
    copy_if_missing(source, &dest)?;

    let title = metadata.title.or_else(|| Some(asset.display_stem));

    Ok(Song {
        hash: hash.clone(),
        file_path: format!("media-g/{}.zip", hash),
        cdg_path: None,
        media_g_container: Some(MEDIA_G_ZIP.to_owned()),
        title,
        artist: metadata.artist,
        album: metadata.album,
        duration_ms: metadata.duration_ms,
        cover_art: metadata.cover_art,
        imported_at,
        original_ext: Some(asset.audio_extension),
    })
}

fn copy_if_missing(source: &Path, destination: &Path) -> Result<()> {
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

fn display_audio_format(ext: &str) -> &str {
    match ext.to_lowercase().as_str() {
        "mp3" => "MP3",
        "flac" => "FLAC",
        "wav" | "wave" => "WAV",
        "ogg" => "OGG",
        "aac" | "m4a" => "AAC/M4A",
        "opus" => "Opus",
        "aiff" | "aif" => "AIFF",
        _ => ext,
    }
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

fn try_extract_embedded_lyrics(connection: &Connection, song: &Song, library: &LibraryRoot) {
    if let Ok(Some(_)) = cache::lyrics::get_lyrics_cache_entry(connection, &song.hash) {
        return;
    }

    let raw_lrc = match song.media_g_container.as_deref() {
        Some(MEDIA_G_ZIP) => {
            let archive_path = library.resolve(&song.file_path);
            match media_g::inspect_zip_for_media_g(&archive_path).and_then(|asset| {
                read_embedded_lyrics_from_bytes(&asset.audio_bytes, &asset.audio_extension)
            }) {
                Ok(Some(lrc)) => lrc,
                _ => return,
            }
        }
        _ => {
            let resolved_path = library.resolve(&song.file_path);
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

fn read_embedded_lyrics_from_bytes(bytes: &[u8], extension: &str) -> Result<Option<String>> {
    let file_type = lofty::file::FileType::from_ext(extension)
        .with_context(|| format!("unsupported embedded audio extension {extension}"))?;
    let reader = lofty::probe::Probe::with_file_type(
        std::io::BufReader::new(std::io::Cursor::new(bytes)),
        file_type,
    )
    .read()
    .context("failed to inspect embedded lyrics in Media+G ZIP")?;

    for tag in reader.tags() {
        if let Some(lyrics) = tag.get_string(&ItemKey::Lyrics) {
            let lyrics = lyrics.trim();
            if !lyrics.is_empty() {
                return Ok(Some(lyrics.to_owned()));
            }
        }
    }

    Ok(None)
}

fn delete_song_from_library(
    connection: &Connection,
    library: &LibraryRoot,
    song_id: &str,
) -> Result<()> {
    let song = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;

    if let Some(container) = song.media_g_container.as_deref() {
        match container {
            MEDIA_G_PAIRED => {
                delete_relative_file(library, &song.file_path)?;
                if let Some(cdg_path) = song.cdg_path.as_deref() {
                    delete_relative_file(library, cdg_path)?;
                }
            }
            MEDIA_G_ZIP => delete_relative_file(library, &song.file_path)?,
            _ => {}
        }
    } else {
        delete_relative_file(library, &song.file_path)?;
    }

    cache::stems::delete_stem_cache_entry(connection, library, song_id).ok();
    connection
        .execute("DELETE FROM lyrics WHERE song_hash = ?1", params![song_id])
        .context("failed to delete cached lyrics for song")?;
    if table_exists(connection, "play_history")? {
        connection
            .execute(
                "DELETE FROM play_history WHERE song_hash = ?1",
                params![song_id],
            )
            .context("failed to delete play history for song")?;
    }
    connection
        .execute("DELETE FROM songs WHERE hash = ?1", params![song_id])
        .context("failed to delete song row from database")?;

    Ok(())
}

fn delete_relative_file(library: &LibraryRoot, relative_path: &str) -> Result<()> {
    let absolute = library.resolve(relative_path);
    if absolute.exists() {
        fs::remove_file(&absolute)
            .with_context(|| format!("failed to remove {}", absolute.display()))?;
    }
    Ok(())
}

fn table_exists(connection: &Connection, table: &str) -> Result<bool> {
    let count: i64 = connection
        .query_row(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = ?1",
            [table],
            |row| row.get(0),
        )
        .context("failed to inspect sqlite tables")?;
    Ok(count > 0)
}

#[derive(Default)]
struct ClassifiedImportPaths {
    audio_paths: Vec<PathBuf>,
    cdg_paths: Vec<PathBuf>,
    zip_paths: Vec<PathBuf>,
}

fn classify_import_paths(paths: &[String]) -> ClassifiedImportPaths {
    let mut classified = ClassifiedImportPaths::default();

    for raw_path in paths {
        let path = PathBuf::from(raw_path);
        match path
            .extension()
            .and_then(|value| value.to_str())
            .map(|value| value.to_ascii_lowercase())
            .as_deref()
        {
            Some("cdg") => classified.cdg_paths.push(path),
            Some("zip") => classified.zip_paths.push(path),
            _ => classified.audio_paths.push(path),
        }
    }

    classified
}

fn build_selected_cdg_lookup(paths: &[PathBuf]) -> HashMap<String, Vec<PathBuf>> {
    let mut by_stem = HashMap::new();
    for path in paths {
        let Some(stem) = path.file_stem().and_then(|value| value.to_str()) else {
            continue;
        };
        by_stem
            .entry(stem.to_ascii_lowercase())
            .or_insert_with(Vec::new)
            .push(path.clone());
    }
    by_stem
}

fn match_cdg_source(
    audio_path: &Path,
    selected_cdg_by_stem: &HashMap<String, Vec<PathBuf>>,
    explicit_cdg_by_audio_path: &HashMap<String, String>,
    skip_cdg_for_audio_paths: &[String],
) -> Option<PathBuf> {
    let audio_key = audio_path.display().to_string();
    if skip_cdg_for_audio_paths
        .iter()
        .any(|path| path == &audio_key)
    {
        return None;
    }

    if let Some(cdg_path) = explicit_cdg_by_audio_path.get(&audio_key) {
        return Some(PathBuf::from(cdg_path));
    }

    let stem = audio_path
        .file_stem()
        .and_then(|value| value.to_str())
        .map(|value| value.to_ascii_lowercase())?;

    if let Some(candidates) = selected_cdg_by_stem.get(&stem) {
        if candidates.len() == 1 {
            return Some(candidates[0].clone());
        }

        return None;
    }

    let sibling_cdg = audio_path.with_extension("cdg");
    if sibling_cdg.is_file() {
        return Some(sibling_cdg);
    }

    None
}
