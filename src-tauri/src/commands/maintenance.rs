use crate::{
    cache,
    commands::error::{database_error, internal_error, CommandResult},
    AppState,
};
use serde::Serialize;
use tauri::State;

#[derive(Debug, Serialize)]
pub struct DeleteStemsResult {
    pub deleted_count: usize,
    pub freed_bytes: u64,
}

#[tauri::command]
pub fn delete_all_stems(state: State<'_, AppState>) -> CommandResult<DeleteStemsResult> {
    let library_root = state.library_root()?;

    // Estimate disk usage before deletion.
    let freed_bytes = cache::stems::estimate_stems_disk_usage(&library_root)
        .map_err(|e| internal_error(format!("failed to estimate stems disk usage: {e}")))?;

    let connection = cache::open_database(&library_root.database_path())
        .map_err(|e| database_error(e.to_string()))?;

    let deleted_count = cache::stems::delete_all_stem_cache_entries(&connection, &library_root)
        .map_err(|e| internal_error(format!("failed to delete all stems: {e}")))?;

    // Clear in-memory separation statuses so the frontend reflects the change.
    if let Ok(mut statuses) = state.separation_statuses.lock() {
        statuses.clear();
    }

    Ok(DeleteStemsResult {
        deleted_count,
        freed_bytes,
    })
}

#[tauri::command]
pub fn estimate_stems_size(state: State<'_, AppState>) -> CommandResult<u64> {
    let library_root = state.library_root()?;
    cache::stems::estimate_stems_disk_usage(&library_root)
        .map_err(|e| internal_error(format!("failed to estimate stems disk usage: {e}")))
}

#[tauri::command]
pub fn delete_all_cached_lyrics(state: State<'_, AppState>) -> CommandResult<usize> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path())
        .map_err(|e| database_error(e.to_string()))?;

    cache::lyrics::delete_all_lyrics_cache_entries(&connection)
        .map_err(|e| database_error(e.to_string()))
}
