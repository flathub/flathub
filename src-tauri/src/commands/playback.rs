use crate::{
    audio::{
        decode, output,
        playback::{
            monotonic_now_ms, playback_position_event, LoadedStems, PlaybackController,
            PlaybackStateSnapshot, StemName, StemSet, PLAYBACK_POSITION_EVENT,
        },
    },
    cache,
    commands::error::{
        database_error, internal_error, playback_error, state_lock_error, CommandResult,
    },
    library_root::LibraryRoot,
    AppState,
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use tauri::{AppHandle, Emitter, State};

#[tauri::command]
pub fn play(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<PlaybackStateSnapshot> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot =
        play_song_from_library(&connection, &library_root, &mut playback, &song_id, monotonic_now_ms())
            .map_err(playback_error)?;
    drop(playback);

    output::ensure_output_thread(
        &state.audio_output_started,
        &state.audio_output_start_lock,
        state.playback.clone(),
    )
    .map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn resume(
    state: State<'_, AppState>,
    app_handle: AppHandle,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot = playback.play(monotonic_now_ms()).map_err(playback_error)?;
    drop(playback);

    output::ensure_output_thread(
        &state.audio_output_started,
        &state.audio_output_start_lock,
        state.playback.clone(),
    )
    .map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn pause(
    state: State<'_, AppState>,
    app_handle: AppHandle,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot = playback.pause(monotonic_now_ms()).map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn seek(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    ms: u64,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot = playback
        .seek(ms, monotonic_now_ms())
        .map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn set_volume(state: State<'_, AppState>, level: f32) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    playback.set_volume(level).map_err(playback_error)
}

#[tauri::command]
pub fn set_stem_volume(
    state: State<'_, AppState>,
    stem: StemName,
    level: f32,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    playback
        .set_stem_volume(stem, level)
        .map_err(playback_error)
}

#[tauri::command]
pub fn load_stems(state: State<'_, AppState>) -> CommandResult<PlaybackStateSnapshot> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    load_stems_for_current_track(&connection, &library_root, &mut playback)
        .map_err(playback_error)?;

    Ok(playback.snapshot(monotonic_now_ms()))
}

#[tauri::command]
pub fn get_playback_state(state: State<'_, AppState>) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    Ok(playback.snapshot(monotonic_now_ms()))
}

pub fn play_song_from_library(
    connection: &Connection,
    library_root: &LibraryRoot,
    controller: &mut PlaybackController,
    song_id: &str,
    now_ms: u64,
) -> Result<PlaybackStateSnapshot> {
    let song = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;
    let absolute_path = library_root.resolve(&song.file_path);
    let decoded_audio = decode::decode_file(&absolute_path)
        .with_context(|| format!("failed to decode audio for {}", song.file_path))?;

    Ok(controller.start_track(song.hash, decoded_audio, now_ms))
}

pub fn load_stems_for_current_track(
    connection: &Connection,
    library_root: &LibraryRoot,
    controller: &mut PlaybackController,
) -> Result<()> {
    let song_id = controller
        .current_song_id()
        .context("no track is loaded")?
        .to_owned();

    if controller.has_stems() {
        return Ok(());
    }

    let cached = cache::stems::get_cached_stem_entry(connection, &song_id)
        .context("failed to load cached stems")?
        .with_context(|| format!("no cached stems for song {song_id}"))?;

    let load_stem = |path: &str| -> Result<decode::DecodedAudio> {
        let abs = library_root.resolve(path);
        decode::decode_file(&abs)
            .with_context(|| format!("failed to decode stem {}", path))
    };

    // Load as 4-stem if individual stems are available, otherwise 2-stem
    let loaded = if cached.has_individual_stems() {
        LoadedStems::FourStem(StemSet {
            vocals: load_stem(&cached.vocals_path)?,
            drums: load_stem(cached.drums_path.as_ref().unwrap())?,
            bass: load_stem(cached.bass_path.as_ref().unwrap())?,
            other: load_stem(cached.other_path.as_ref().unwrap())?,
        })
    } else {
        LoadedStems::TwoStem {
            vocals: load_stem(&cached.vocals_path)?,
            accompaniment: load_stem(&cached.accomp_path)?,
        }
    };

    controller.attach_stems(&song_id, loaded)?;

    Ok(())
}

pub fn emit_playback_position(
    app_handle: &AppHandle,
    snapshot: &PlaybackStateSnapshot,
) -> tauri::Result<()> {
    if snapshot.song_id.is_none() {
        return Ok(());
    }

    app_handle.emit(
        PLAYBACK_POSITION_EVENT,
        playback_position_event(snapshot).map_err(|error| tauri::Error::Anyhow(error.into()))?,
    )
}
