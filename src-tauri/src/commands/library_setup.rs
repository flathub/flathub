use crate::{
    cache,
    commands::error::{library_error, state_lock_error, CommandResult},
    config::{self, AppConfig},
    library_root::LibraryRoot,
    AppState,
};
use std::path::PathBuf;
use tauri::State;

#[tauri::command]
pub fn create_library(state: State<'_, AppState>, path: String) -> CommandResult<()> {
    let lib_path = PathBuf::from(&path);

    let lib = LibraryRoot::create(&lib_path).map_err(library_error)?;

    let db_path = lib.database_path();
    cache::initialize_library_database(&db_path).map_err(library_error)?;

    config::save_config(
        &state.app_data_dir,
        &AppConfig {
            library_path: Some(path),
            stem_mode: None,
            language: None,
            hide_batch_separate: None,
            model_variant: None,
            lyrics_font_step: None,
        },
    )
    .map_err(library_error)?;

    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(lib);

    Ok(())
}

#[tauri::command]
pub fn open_library(state: State<'_, AppState>, path: String) -> CommandResult<()> {
    let lib_path = PathBuf::from(&path);

    let lib = LibraryRoot::open(&lib_path).map_err(library_error)?;

    let db_path = lib.database_path();
    cache::initialize_library_database(&db_path).map_err(library_error)?;

    config::save_config(
        &state.app_data_dir,
        &AppConfig {
            library_path: Some(path),
            stem_mode: None,
            language: None,
            hide_batch_separate: None,
            model_variant: None,
            lyrics_font_step: None,
        },
    )
    .map_err(library_error)?;

    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(lib);

    Ok(())
}

#[tauri::command]
pub fn get_library_path(state: State<'_, AppState>) -> CommandResult<Option<String>> {
    let guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;

    Ok(guard.as_ref().map(|lib| lib.root().display().to_string()))
}
