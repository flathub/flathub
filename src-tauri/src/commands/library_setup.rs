use crate::{
    cache,
    commands::error::{library_error, state_lock_error, CommandResult},
    config::{self, AppConfig, RegisteredLibrary},
    library_root::LibraryRoot,
    AppState,
};
use serde::Serialize;
use std::{
    fs,
    path::{Path, PathBuf},
};
use tauri::{AppHandle, Manager, State};

#[derive(Debug, Clone, Serialize)]
pub struct LibraryRegistrySnapshot {
    pub active_library_id: Option<String>,
    pub libraries: Vec<RegisteredLibrary>,
}

fn canonical_path_string(path: &Path) -> String {
    fs::canonicalize(path)
        .unwrap_or_else(|_| path.to_path_buf())
        .display()
        .to_string()
}

fn load_app_config(app_data_dir: &Path) -> CommandResult<AppConfig> {
    Ok(config::load_config(app_data_dir)
        .map_err(library_error)?
        .unwrap_or_default())
}

fn persist_app_config(app_data_dir: &Path, config: &AppConfig) -> CommandResult<()> {
    config::save_config(app_data_dir, config).map_err(library_error)
}

fn upsert_library(config: &mut AppConfig, library: RegisteredLibrary) {
    if let Some(existing) = config
        .libraries
        .iter_mut()
        .find(|entry| entry.id() == library.id())
    {
        *existing = library;
    } else {
        config.libraries.push(library);
    }
}

fn set_active_library(config: &mut AppConfig, library_id: String) {
    config.active_library_id = Some(library_id);
    config.library_path = None;
}

fn store_active_library(
    state: &State<'_, AppState>,
    config: &mut AppConfig,
    library: LibraryRoot,
) -> CommandResult<()> {
    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(library);
    config.library_path = None;
    Ok(())
}

fn register_library(
    state: &State<'_, AppState>,
    app_data_dir: &Path,
    library: RegisteredLibrary,
    root: LibraryRoot,
) -> CommandResult<LibraryRegistrySnapshot> {
    let db_path = root.database_path();
    cache::initialize_library_database(&db_path).map_err(library_error)?;

    let mut config = load_app_config(app_data_dir)?;
    upsert_library(&mut config, library.clone());
    set_active_library(&mut config, library.id().to_owned());
    persist_app_config(app_data_dir, &config)?;

    store_active_library(state, &mut config, root)?;
    {
        let mut upload_statuses = state
            .remote_upload_statuses
            .lock()
            .map_err(|_| state_lock_error("remote upload status lock was poisoned"))?;
        upload_statuses.clear();
    }

    Ok(LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

fn activate_library(
    state: &State<'_, AppState>,
    app_data_dir: &Path,
    library_id: &str,
) -> CommandResult<LibraryRegistrySnapshot> {
    let mut config = load_app_config(app_data_dir)?;
    let library = config
        .libraries
        .iter()
        .find(|entry| entry.id() == library_id)
        .cloned()
        .ok_or_else(|| library_error(format!("library {library_id} was not found")))?;

    let root_path = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let lib = LibraryRoot::open(&root_path).map_err(library_error)?;
    let db_path = lib.database_path();
    cache::initialize_library_database(&db_path).map_err(library_error)?;

    {
        let mut playback = state
            .playback
            .lock()
            .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
        playback.clear_track();
    }
    {
        let mut cdg_state = state
            .cdg_state
            .lock()
            .map_err(|_| state_lock_error("CDG state lock was poisoned"))?;
        *cdg_state = None;
    }
    {
        let mut upload_statuses = state
            .remote_upload_statuses
            .lock()
            .map_err(|_| state_lock_error("remote upload status lock was poisoned"))?;
        upload_statuses.clear();
    }

    set_active_library(&mut config, library.id().to_owned());
    persist_app_config(app_data_dir, &config)?;
    store_active_library(state, &mut config, lib)?;

    Ok(LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

#[tauri::command]
pub fn create_library(
    state: State<'_, AppState>,
    path: String,
) -> CommandResult<LibraryRegistrySnapshot> {
    let lib_path = PathBuf::from(&path);

    let lib = LibraryRoot::create(&lib_path).map_err(library_error)?;
    let canonical_root = canonical_path_string(lib.root());
    let library = RegisteredLibrary::local(
        canonical_root.clone(),
        config::library_display_name(&canonical_root),
    );

    register_library(&state, &state.app_data_dir, library, lib)
}

#[tauri::command]
pub fn open_library(
    state: State<'_, AppState>,
    path: String,
) -> CommandResult<LibraryRegistrySnapshot> {
    let lib_path = PathBuf::from(&path);

    let lib = LibraryRoot::open(&lib_path).map_err(library_error)?;
    let canonical_root = canonical_path_string(lib.root());
    let library = RegisteredLibrary::local(
        canonical_root.clone(),
        config::library_display_name(&canonical_root),
    );

    register_library(&state, &state.app_data_dir, library, lib)
}

#[tauri::command]
pub fn switch_library(
    state: State<'_, AppState>,
    library_id: String,
) -> CommandResult<LibraryRegistrySnapshot> {
    activate_library(&state, &state.app_data_dir, &library_id)
}

#[tauri::command]
pub fn get_library_path(state: State<'_, AppState>) -> CommandResult<Option<String>> {
    let guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;

    Ok(guard.as_ref().map(|lib| canonical_path_string(lib.root())))
}

#[tauri::command]
pub fn get_library_registry(app_handle: AppHandle) -> CommandResult<LibraryRegistrySnapshot> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| library_error(error.to_string()))?;
    let config = load_app_config(&app_data_dir)?;

    Ok(LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

#[tauri::command]
pub fn get_active_library(app_handle: AppHandle) -> CommandResult<Option<RegisteredLibrary>> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| library_error(error.to_string()))?;
    let config = load_app_config(&app_data_dir)?;

    Ok(config.active_library().cloned())
}

#[tauri::command]
pub fn remove_library(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    library_id: String,
) -> CommandResult<LibraryRegistrySnapshot> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| library_error(error.to_string()))?;
    let mut config = load_app_config(&app_data_dir)?;
    let removed_active = config.active_library_id.as_deref() == Some(library_id.as_str());
    let removed_libraries: Vec<_> = config
        .libraries
        .iter()
        .filter(|library| library.id() == library_id)
        .cloned()
        .collect();
    config.libraries.retain(|library| library.id() != library_id);

    for library in &removed_libraries {
        crate::commands::remote_library::remove_remote_library_credentials(&app_data_dir, library)?;
    }

    if removed_active {
        config.active_library_id = config
            .libraries
            .first()
            .map(|library| library.id().to_owned());
    }

    persist_app_config(&app_data_dir, &config)?;

    if config.active_library_id.is_none() {
        let mut guard = state
            .library
            .lock()
            .map_err(|_| state_lock_error("library lock was poisoned"))?;
        *guard = None;
        {
            let mut playback = state
                .playback
                .lock()
                .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
            playback.clear_track();
        }
        {
            let mut cdg_state = state
                .cdg_state
                .lock()
                .map_err(|_| state_lock_error("CDG state lock was poisoned"))?;
            *cdg_state = None;
        }
        {
            let mut upload_statuses = state
                .remote_upload_statuses
                .lock()
                .map_err(|_| state_lock_error("remote upload status lock was poisoned"))?;
            upload_statuses.clear();
        }
    } else if removed_active {
        activate_library(
            &state,
            &app_data_dir,
            config.active_library_id.as_deref().unwrap_or_default(),
        )?;
    }

    Ok(LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn register_local_library_sets_active_library() {
        let mut config = AppConfig::default();
        let library = RegisteredLibrary::local("/tmp/library".to_owned(), "library".to_owned());
        upsert_library(&mut config, library.clone());
        set_active_library(&mut config, library.id().to_owned());
        assert_eq!(config.active_library_id.as_deref(), Some(library.id()));
        assert_eq!(config.libraries.len(), 1);
    }

    #[test]
    fn register_library_replaces_existing_entry_with_same_id() {
        let mut config = AppConfig::default();
        let first = RegisteredLibrary::local("/tmp/library".to_owned(), "one".to_owned());
        let second = RegisteredLibrary::local("/tmp/library".to_owned(), "two".to_owned());
        upsert_library(&mut config, first);
        upsert_library(&mut config, second.clone());
        assert_eq!(config.libraries.len(), 1);
        assert_eq!(config.libraries[0].display_name(), second.display_name());
    }
}
