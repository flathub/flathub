use crate::{
    cache,
    commands::{
        error::{library_error, state_lock_error, CommandResult},
        library_setup::LibraryRegistrySnapshot,
    },
    config::{RegisteredLibrary, RemoteLibraryConnectionConfig, RemoteLibraryProvider},
    library_root::LibraryRoot,
    AppState,
};
use std::{fs, path::Path};
use tauri::AppHandle;

use super::{
    dropbox::{
        dropbox_ensure_folder_with_token, initialize_or_sync_dropbox_library,
        load_dropbox_secret, normalize_dropbox_root_path, store_dropbox_secret,
    },
    google_drive::{
        google_drive_get_or_create_folder_with_token, google_drive_root_display_name,
        initialize_or_sync_google_drive_library, load_google_drive_secret,
        store_google_drive_secret, GOOGLE_DRIVE_ROOT_ID,
    },
    types::{
        current_unix_time_ms, delete_remote_credential, load_app_config, persist_app_config,
        remote_libraries_dir, remote_library_id, remote_library_root, DropboxSecret,
        GoogleDriveSecret, RemoteAuthSession, RemoteAuthState, RemoteLibraryCandidate,
    },
    webdav::{
        initialize_or_sync_webdav_library, join_url, load_webdav_secret, normalize_webdav_root_path,
        remote_path_display_from_url, store_webdav_secret,
    },
};

fn candidate_from_session(
    session_id: &str,
    session: &RemoteAuthSession,
    display_name: &str,
) -> RemoteLibraryCandidate {
    let remote_root_locator = session
        .remote_root_locator
        .clone()
        .unwrap_or_else(|| format!("{}:{}", session.provider.as_str(), session_id));
    RemoteLibraryCandidate {
        provider: session.provider,
        remote_root_locator: remote_root_locator.clone(),
        remote_path_display: match session.provider {
            RemoteLibraryProvider::WebDav => remote_path_display_from_url(&remote_root_locator),
            RemoteLibraryProvider::GoogleDrive => google_drive_root_display_name(display_name),
            RemoteLibraryProvider::Dropbox => remote_root_locator.clone(),
        },
        display_name: display_name.to_owned(),
        account_id: session.account_id.clone(),
    }
}

pub(crate) fn list_remote_library_roots(
    state: &AppState,
    session_id: String,
) -> CommandResult<Vec<RemoteLibraryCandidate>> {
    let sessions = state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?;
    let session = sessions
        .get(&session_id)
        .ok_or_else(|| library_error(format!("remote auth session {session_id} was not found")))?;

    if let (Some(remote_root_locator), Some(display_name)) = (
        session.remote_root_locator.clone(),
        session.display_name.clone(),
    ) {
        return Ok(vec![RemoteLibraryCandidate {
            provider: session.provider,
            remote_root_locator: remote_root_locator.clone(),
            remote_path_display: match session.provider {
                RemoteLibraryProvider::WebDav => remote_path_display_from_url(&remote_root_locator),
                RemoteLibraryProvider::GoogleDrive => {
                    google_drive_root_display_name(&display_name)
                }
                RemoteLibraryProvider::Dropbox => remote_root_locator,
            },
            display_name,
            account_id: session.account_id.clone(),
        }]);
    }

    Ok(Vec::new())
}

pub(crate) fn create_remote_library(
    state: &AppState,
    session_id: String,
    display_name: String,
) -> CommandResult<RemoteLibraryCandidate> {
    let mut sessions = state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?;
    let session = sessions
        .get_mut(&session_id)
        .ok_or_else(|| library_error(format!("remote auth session {session_id} was not found")))?;

    let remote_root_locator = match session.provider {
        RemoteLibraryProvider::GoogleDrive => {
            let google = session
                .google_drive
                .as_mut()
                .ok_or_else(|| library_error("missing Google Drive session details".to_owned()))?;
            let access_token = google.access_token.clone().ok_or_else(|| {
                library_error(
                    "Google Drive sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let root = google_drive_get_or_create_folder_with_token(
                &access_token,
                GOOGLE_DRIVE_ROOT_ID,
                &display_name,
            )?;
            google.root_folder_id = Some(root.id.clone());
            root.id
        }
        RemoteLibraryProvider::Dropbox => {
            let dropbox = session
                .dropbox
                .as_mut()
                .ok_or_else(|| library_error("missing Dropbox session details".to_owned()))?;
            let access_token = dropbox.access_token.clone().ok_or_else(|| {
                library_error(
                    "Dropbox sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let root_path = normalize_dropbox_root_path(None, &display_name);
            dropbox_ensure_folder_with_token(&access_token, &root_path)?;
            root_path
        }
        RemoteLibraryProvider::WebDav => {
            let webdav = session
                .webdav
                .as_ref()
                .ok_or_else(|| library_error("missing WebDAV session details".to_owned()))?;
            let root_path =
                normalize_webdav_root_path(webdav.root_path.as_deref(), &display_name);
            join_url(&webdav.server_url, &format!("{root_path}/"))?
        }
    };
    session.state = RemoteAuthState::Ready;
    session.remote_root_locator = Some(remote_root_locator.clone());
    session.display_name = Some(display_name.clone());

    Ok(candidate_from_session(&session_id, session, &display_name))
}

pub(crate) fn register_remote_library(
    state: &AppState,
    app_data_dir: &Path,
    session_id: String,
    remote_root_locator: String,
    display_name: Option<String>,
) -> CommandResult<LibraryRegistrySnapshot> {
    let mut sessions = state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?;
    let (default_display_name, account_id, provider, webdav, google_drive, dropbox) = {
        let session = sessions
            .get(&session_id)
            .ok_or_else(|| library_error(format!("remote auth session {session_id} was not found")))?;
        (
            session.display_name.clone(),
            session.account_id.clone(),
            session.provider,
            session.webdav.clone(),
            session.google_drive.clone(),
            session.dropbox.clone(),
        )
    };

    let display_name = display_name
        .or(default_display_name)
        .unwrap_or_else(|| "Remote Library".to_owned());
    if let Some(session) = sessions.get_mut(&session_id) {
        session.state = RemoteAuthState::Ready;
        session.remote_root_locator = Some(remote_root_locator.clone());
        session.display_name = Some(display_name.clone());
    }
    drop(sessions);

    fs::create_dir_all(remote_libraries_dir(app_data_dir)).map_err(|error| {
        library_error(format!(
            "failed to create remote library root at {}: {error}",
            remote_libraries_dir(app_data_dir).display()
        ))
    })?;

    let library_id = remote_library_id(provider, &account_id, &remote_root_locator);
    let root_path = remote_library_root(app_data_dir, &library_id);
    let library_root = if root_path.join(".openkara-library").exists() {
        LibraryRoot::open(&root_path).map_err(library_error)?
    } else {
        LibraryRoot::create(&root_path).map_err(library_error)?
    };
    cache::initialize_library_database(&library_root.database_path()).map_err(library_error)?;

    let remote_path_display = if provider == RemoteLibraryProvider::WebDav {
        remote_path_display_from_url(&remote_root_locator)
    } else if provider == RemoteLibraryProvider::GoogleDrive {
        google_drive_root_display_name(&display_name)
    } else {
        remote_root_locator.clone()
    };
    let connection_config = match provider {
        RemoteLibraryProvider::GoogleDrive => Some(RemoteLibraryConnectionConfig::GoogleDrive {
            oauth_client_id: google_drive
                .as_ref()
                .map(|session| session.client_id.clone())
                .ok_or_else(|| {
                    library_error(
                        "missing Google Drive session details during registration".to_owned(),
                    )
                })?,
        }),
        RemoteLibraryProvider::Dropbox => Some(RemoteLibraryConnectionConfig::Dropbox {
            app_key: dropbox
                .as_ref()
                .map(|session| session.app_key.clone())
                .ok_or_else(|| {
                    library_error("missing Dropbox session details during registration".to_owned())
                })?,
        }),
        RemoteLibraryProvider::WebDav => Some(RemoteLibraryConnectionConfig::WebDav {
            server_url: webdav
                .as_ref()
                .map(|session| session.server_url.clone())
                .ok_or_else(|| {
                    library_error("missing WebDAV session details during registration".to_owned())
                })?,
        }),
    };
    let provisional_library = RegisteredLibrary::remote(
        library_id.clone(),
        display_name.clone(),
        provider,
        account_id.clone(),
        remote_root_locator.clone(),
        remote_path_display.clone(),
        connection_config.clone(),
        Some(library_root.database_path().display().to_string()),
        None,
        None,
    );
    let remote_revision = match provider {
        RemoteLibraryProvider::GoogleDrive => {
            let google = google_drive.clone().ok_or_else(|| {
                library_error("missing Google Drive session details during registration".to_owned())
            })?;
            let access_token = google.access_token.clone().ok_or_else(|| {
                library_error(
                    "Google Drive sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let refresh_token = google.refresh_token.clone().ok_or_else(|| {
                library_error(
                    "Google Drive did not return a refresh token. Reconnect and ensure consent was granted."
                        .to_owned(),
                )
            })?;
            store_google_drive_secret(
                app_data_dir,
                GoogleDriveSecret {
                    library_id: library_id.clone(),
                    client_id: google.client_id,
                    client_secret: google.client_secret,
                    access_token,
                    refresh_token,
                    access_token_expires_at_ms: google.access_token_expires_at_ms,
                },
            )?;
            let secret = load_google_drive_secret(app_data_dir, &provisional_library)?;
            initialize_or_sync_google_drive_library(app_data_dir, &provisional_library, &secret)?
        }
        RemoteLibraryProvider::Dropbox => {
            let dropbox = dropbox.clone().ok_or_else(|| {
                library_error("missing Dropbox session details during registration".to_owned())
            })?;
            let access_token = dropbox.access_token.clone().ok_or_else(|| {
                library_error(
                    "Dropbox sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let refresh_token = dropbox.refresh_token.clone().ok_or_else(|| {
                library_error(
                    "Dropbox did not return a refresh token. Reconnect and ensure consent was granted."
                        .to_owned(),
                )
            })?;
            store_dropbox_secret(
                app_data_dir,
                DropboxSecret {
                    library_id: library_id.clone(),
                    app_key: dropbox.app_key,
                    app_secret: dropbox.app_secret,
                    access_token,
                    refresh_token,
                    access_token_expires_at_ms: dropbox.access_token_expires_at_ms,
                },
            )?;
            let secret = load_dropbox_secret(app_data_dir, &provisional_library)?;
            initialize_or_sync_dropbox_library(app_data_dir, &provisional_library, &secret)?
        }
        RemoteLibraryProvider::WebDav => {
            let webdav = webdav.ok_or_else(|| {
                library_error("missing WebDAV session details during registration".to_owned())
            })?;
            store_webdav_secret(app_data_dir, &library_id, webdav.username, webdav.password)?;
            let secret = load_webdav_secret(app_data_dir, &provisional_library)?;
            initialize_or_sync_webdav_library(app_data_dir, &provisional_library, &secret)?
        }
    };
    let library = RegisteredLibrary::remote(
        library_id.clone(),
        display_name.clone(),
        provider,
        account_id,
        remote_root_locator,
        remote_path_display,
        connection_config,
        Some(library_root.database_path().display().to_string()),
        remote_revision.or_else(|| Some(current_unix_time_ms().to_string())),
        None,
    );
    let mut config = load_app_config(app_data_dir)?;

    if let Some(existing) = config
        .libraries
        .iter_mut()
        .find(|entry| entry.id() == library.id())
    {
        *existing = library.clone();
    } else {
        config.libraries.push(library.clone());
    }
    config.active_library_id = Some(library.id().to_owned());
    persist_app_config(app_data_dir, &config)?;

    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(library_root);
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

pub(crate) fn set_remote_mirror(
    state: &AppState,
    app_handle: &AppHandle,
    app_data_dir: &Path,
    local_library_id: String,
    remote_library_id: Option<String>,
) -> CommandResult<LibraryRegistrySnapshot> {
    let mut config = load_app_config(app_data_dir)?;

    if let Some(local_library) = config
        .libraries
        .iter()
        .find(|entry| entry.id() == local_library_id)
    {
        if !matches!(local_library, RegisteredLibrary::Local { .. }) {
            return Err(library_error(
                "the local library id must point to a local library",
            ));
        }
    } else {
        return Err(library_error(format!(
            "local library {local_library_id} was not found"
        )));
    }

    for library in &mut config.libraries {
        if let RegisteredLibrary::Remote {
            bound_local_library_id,
            ..
        } = library
        {
            if bound_local_library_id.as_deref() == Some(local_library_id.as_str()) {
                *bound_local_library_id = None;
            }
        }
    }

    if let Some(remote_library_id) = remote_library_id.clone() {
        let Some(remote_library) = config
            .libraries
            .iter_mut()
            .find(|entry| entry.id() == remote_library_id)
        else {
            return Err(library_error(format!(
                "remote library {remote_library_id} was not found"
            )));
        };

        if let RegisteredLibrary::Remote {
            bound_local_library_id,
            ..
        } = remote_library
        {
            *bound_local_library_id = Some(local_library_id.clone());
        } else {
            return Err(library_error("the target library must be remote"));
        }
    }

    persist_app_config(app_data_dir, &config)?;

    if remote_library_id.is_some()
        && config.active_library_id.as_deref() == Some(local_library_id.as_str())
    {
        super::sync::sync_bound_remote_for_active_local_library(state, app_handle)?;
    }

    Ok(LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

pub(crate) fn remove_remote_library_credentials(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<()> {
    if !matches!(library, RegisteredLibrary::Remote { .. }) {
        return Ok(());
    }
    delete_remote_credential(app_data_dir, library.id())?;
    Ok(())
}
