use crate::{
    audio::playback::{PlaybackStateSnapshot, StemName},
    commands::error::{playback_error, CommandResult},
    services, AppState,
};
use tauri::{AppHandle, State};

pub use crate::services::playback::play_song_from_library;

#[tauri::command]
pub fn play(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::play(&state, &app_handle, &song_id).map_err(playback_error)
}

#[tauri::command]
pub fn resume(
    state: State<'_, AppState>,
    app_handle: AppHandle,
) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::resume(&state, &app_handle).map_err(playback_error)
}

#[tauri::command]
pub fn pause(
    state: State<'_, AppState>,
    app_handle: AppHandle,
) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::pause(&state, &app_handle).map_err(playback_error)
}

#[tauri::command]
pub fn seek(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    ms: u64,
) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::seek(&state, &app_handle, ms).map_err(playback_error)
}

#[tauri::command]
pub fn set_volume(state: State<'_, AppState>, level: f32) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::set_volume(&state, level).map_err(playback_error)
}

#[tauri::command]
pub fn set_stem_volume(
    state: State<'_, AppState>,
    stem: StemName,
    level: f32,
) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::set_stem_volume(&state, stem, level).map_err(playback_error)
}

#[tauri::command]
pub fn load_stems(state: State<'_, AppState>) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::load_stems(&state).map_err(playback_error)
}

#[tauri::command]
pub fn get_playback_state(state: State<'_, AppState>) -> CommandResult<PlaybackStateSnapshot> {
    services::playback::get_state(&state).map_err(playback_error)
}
