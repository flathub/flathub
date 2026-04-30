mod auth;
mod dropbox;
mod google_drive;
mod registry;
mod sync;
mod types;
mod webdav;

use crate::{
    commands::error::{library_error, CommandResult},
    config::{RegisteredLibrary, RemoteLibraryProvider},
    AppState,
};
use tauri::{AppHandle, Manager, State};

pub(crate) use registry::remove_remote_library_credentials;
pub(crate) use sync::{
    ensure_remote_file_cached, maybe_publish_song_to_bound_remote,
    maybe_publish_songs_to_bound_remote, prepare_active_remote_database_for_mutation,
    sync_active_remote_database_if_needed, sync_bound_remote_for_active_local_library,
};
pub use types::{
    RemoteAuthSession, RemoteAuthStart, RemoteAuthState, RemoteAuthStatus,
    RemoteLibraryCandidate, UploadState, UploadStatusSnapshot,
};

#[tauri::command]
pub fn begin_remote_auth(
    state: State<'_, AppState>,
    provider: RemoteLibraryProvider,
    payload: Option<serde_json::Value>,
) -> CommandResult<RemoteAuthStart> {
    auth::begin_remote_auth(&state, provider, payload)
}

#[tauri::command]
pub fn poll_remote_auth(
    state: State<'_, AppState>,
    session_id: String,
) -> CommandResult<RemoteAuthStatus> {
    auth::poll_remote_auth(&state, session_id)
}

#[tauri::command]
pub fn cancel_remote_auth(state: State<'_, AppState>, session_id: String) -> CommandResult<()> {
    auth::cancel_remote_auth(&state, session_id)
}

#[tauri::command]
pub fn open_external_url(url: String) -> CommandResult<()> {
    auth::open_external_url(url)
}

#[tauri::command]
pub fn list_remote_library_roots(
    state: State<'_, AppState>,
    session_id: String,
) -> CommandResult<Vec<RemoteLibraryCandidate>> {
    registry::list_remote_library_roots(&state, session_id)
}

#[tauri::command]
pub fn create_remote_library(
    state: State<'_, AppState>,
    session_id: String,
    display_name: String,
) -> CommandResult<RemoteLibraryCandidate> {
    registry::create_remote_library(&state, session_id, display_name)
}

#[tauri::command]
pub fn register_remote_library(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    session_id: String,
    remote_root_locator: String,
    display_name: Option<String>,
) -> CommandResult<crate::commands::library_setup::LibraryRegistrySnapshot> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| library_error(error.to_string()))?;
    registry::register_remote_library(
        &state,
        &app_data_dir,
        session_id,
        remote_root_locator,
        display_name,
    )
}

#[tauri::command]
pub fn mirror_local_library_to_remote(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    local_library_id: String,
    remote_library_id: String,
) -> CommandResult<()> {
    sync::mirror_local_library_to_remote(&state, &app_handle, &local_library_id, &remote_library_id)
}

#[tauri::command]
pub fn sync_active_remote_library(state: State<'_, AppState>) -> CommandResult<()> {
    sync::sync_active_remote_library(&state)
}

#[tauri::command]
pub fn publish_song_to_remote(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<UploadStatusSnapshot> {
    sync::publish_song_to_remote(&state, &app_handle, song_id)
}

#[tauri::command]
pub fn publish_songs_to_remote(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_ids: Vec<String>,
) -> CommandResult<Vec<UploadStatusSnapshot>> {
    sync::publish_songs_to_remote(&state, &app_handle, song_ids)
}

#[tauri::command]
pub fn get_all_upload_statuses(
    state: State<'_, AppState>,
) -> CommandResult<Vec<UploadStatusSnapshot>> {
    sync::get_all_upload_statuses(&state)
}

pub(crate) fn delete_remote_library_root(
    app_data_dir: &std::path::Path,
    library: &RegisteredLibrary,
) -> CommandResult<()> {
    match library.provider() {
        Some(RemoteLibraryProvider::GoogleDrive) => {
            google_drive::delete_remote_root(app_data_dir, library)
        }
        Some(RemoteLibraryProvider::Dropbox) => dropbox::delete_remote_root(app_data_dir, library),
        Some(RemoteLibraryProvider::WebDav) => webdav::delete_remote_root(app_data_dir, library),
        None => Err(library_error(
            "the target library must be remote".to_owned(),
        )),
    }
}
