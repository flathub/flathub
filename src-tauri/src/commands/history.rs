use crate::{
    cache,
    commands::error::{database_error, CommandResult},
    AppState,
};
use tauri::State;

#[tauri::command]
pub fn record_play(state: State<'_, AppState>, song_id: String) -> CommandResult<()> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    cache::history::record_play(&connection, &song_id).map_err(database_error)?;
    Ok(())
}

#[tauri::command]
pub fn get_play_history(state: State<'_, AppState>, limit: u32) -> CommandResult<Vec<String>> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    let hashes = cache::history::get_recent_plays(&connection, limit).map_err(database_error)?;
    Ok(hashes)
}
