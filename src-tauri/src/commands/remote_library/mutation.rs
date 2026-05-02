use crate::{
    commands::error::CommandResult,
    library::Song,
    AppState,
};
use tauri::AppHandle;

use super::sync;

pub(crate) fn run_imported_songs_mutation<R, F>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    mutation: F,
) -> CommandResult<crate::library::ImportSongsResult>
where
    R: tauri::Runtime,
    F: FnOnce() -> crate::library::ImportSongsResult,
{
    sync::prepare_active_remote_database_for_mutation(&state.app_data_dir)?;
    let result = mutation();
    let imported_song_ids: Vec<String> =
        result.imported.iter().map(|song| song.hash.clone()).collect();
    sync::maybe_publish_songs_to_bound_remote(state, app_handle, &imported_song_ids)?;
    Ok(result)
}

pub(crate) fn run_updated_songs_mutation<R, T, F, S>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    mutation: F,
    updated_song_ids: S,
) -> CommandResult<T>
where
    R: tauri::Runtime,
    F: FnOnce() -> CommandResult<T>,
    S: FnOnce(&T) -> Vec<String>,
{
    sync::prepare_active_remote_database_for_mutation(&state.app_data_dir)?;
    let result = mutation()?;
    let song_ids = updated_song_ids(&result);
    sync::maybe_publish_songs_to_bound_remote(state, app_handle, &song_ids)?;
    Ok(result)
}

pub(crate) fn publish_song_to_active_remote_if_ready<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: &str,
) -> CommandResult<()> {
    sync::maybe_publish_song_to_bound_remote(state, app_handle, song_id)
}

pub(crate) fn run_song_database_mutation<R, T, F>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: &str,
    mutation: F,
) -> CommandResult<T>
where
    R: tauri::Runtime,
    F: FnOnce() -> CommandResult<T>,
{
    sync::prepare_active_remote_database_for_mutation(&state.app_data_dir)?;
    let result = mutation()?;
    sync::sync_active_remote_database_if_needed(&state.app_data_dir)?;
    sync::maybe_publish_song_to_bound_remote(state, app_handle, song_id)?;
    Ok(result)
}

pub(crate) fn run_song_database_mutation_with_result<R, T, F, S>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    mutation: F,
    song_id: S,
) -> CommandResult<T>
where
    R: tauri::Runtime,
    F: FnOnce() -> CommandResult<T>,
    S: FnOnce(&T) -> Option<String>,
{
    sync::prepare_active_remote_database_for_mutation(&state.app_data_dir)?;
    let result = mutation()?;
    if let Some(song_id) = song_id(&result) {
        sync::sync_active_remote_database_if_needed(&state.app_data_dir)?;
        sync::maybe_publish_song_to_bound_remote(state, app_handle, &song_id)?;
    }
    Ok(result)
}

pub(crate) fn run_songs_database_mutation<R, T, F, S>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    mutation: F,
    song_ids: S,
) -> CommandResult<T>
where
    R: tauri::Runtime,
    F: FnOnce() -> CommandResult<T>,
    S: FnOnce(&T) -> Vec<String>,
{
    sync::prepare_active_remote_database_for_mutation(&state.app_data_dir)?;
    let result = mutation()?;
    let song_ids = song_ids(&result);
    if !song_ids.is_empty() {
        sync::sync_active_remote_database_if_needed(&state.app_data_dir)?;
        sync::maybe_publish_songs_to_bound_remote(state, app_handle, &song_ids)?;
    }
    Ok(result)
}

pub(crate) fn run_active_library_mirror_mutation<R, T, F>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    mutation: F,
) -> CommandResult<T>
where
    R: tauri::Runtime,
    F: FnOnce() -> CommandResult<T>,
{
    let result = mutation()?;
    sync::sync_bound_remote_for_active_local_library(state, app_handle)?;
    Ok(result)
}

pub(crate) fn run_database_then_library_mirror_mutation<R, T, F>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    mutation: F,
) -> CommandResult<T>
where
    R: tauri::Runtime,
    F: FnOnce() -> CommandResult<T>,
{
    sync::prepare_active_remote_database_for_mutation(&state.app_data_dir)?;
    let result = mutation()?;
    sync::sync_active_remote_database_if_needed(&state.app_data_dir)?;
    sync::sync_bound_remote_for_active_local_library(state, app_handle)?;
    Ok(result)
}

pub(crate) fn song_ids_from_songs(songs: &[Song]) -> Vec<String> {
    songs.iter().map(|song| song.hash.clone()).collect()
}
