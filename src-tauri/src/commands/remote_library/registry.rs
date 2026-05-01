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

use super::{
    dropbox::{
        dropbox_ensure_folder_with_token, initialize_or_sync_dropbox_library,
        load_dropbox_secret, normalize_dropbox_root_path, refresh_existing_dropbox_library,
        store_dropbox_secret,
    },
    google_drive::{
        google_drive_get_or_create_folder_with_token, google_drive_root_display_name,
        initialize_or_sync_google_drive_library, load_google_drive_secret,
        refresh_existing_google_drive_library, store_google_drive_secret, GOOGLE_DRIVE_ROOT_ID,
    },
    types::{
        current_unix_time_ms, delete_remote_credential, load_app_config, persist_app_config,
        remote_libraries_dir, remote_library_id, remote_library_root, DropboxSecret,
        GoogleDriveSecret, RemoteAuthSession, RemoteAuthState, RemoteLibraryCandidate, WebDavSecret,
    },
    webdav::{
        initialize_or_sync_webdav_library, join_url, load_webdav_secret, normalize_webdav_root_path,
        refresh_existing_webdav_library, remote_path_display_from_url, store_webdav_secret,
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

pub(crate) fn resolve_remote_library_candidate(
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
                .as_ref()
                .ok_or_else(|| library_error("missing Google Drive session details".to_owned()))?;
            google.root_folder_id.clone().ok_or_else(|| {
                library_error(
                    "Google Drive reauthorization did not resolve a remote repository folder."
                        .to_owned(),
                )
            })?
        }
        RemoteLibraryProvider::Dropbox => session
            .remote_root_locator
            .clone()
            .unwrap_or_else(|| normalize_dropbox_root_path(None, &display_name)),
        RemoteLibraryProvider::WebDav => {
            let webdav = session
                .webdav
                .as_ref()
                .ok_or_else(|| library_error("missing WebDAV session details".to_owned()))?;
            let root_path = normalize_webdav_root_path(webdav.root_path.as_deref(), &display_name);
            join_url(&webdav.server_url, &format!("{root_path}/"))?
        }
    };
    session.state = RemoteAuthState::Ready;
    session.remote_root_locator = Some(remote_root_locator);
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
        .unwrap_or_else(|| "Remote Repository".to_owned());
    if let Some(session) = sessions.get_mut(&session_id) {
        session.state = RemoteAuthState::Ready;
        session.remote_root_locator = Some(remote_root_locator.clone());
        session.display_name = Some(display_name.clone());
    }
    drop(sessions);

    fs::create_dir_all(remote_libraries_dir(app_data_dir)).map_err(|error| {
        library_error(format!(
            "failed to create remote repository root at {}: {error}",
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

pub(crate) fn reauthorize_remote_library(
    state: &AppState,
    app_data_dir: &Path,
    library_id: String,
    session_id: String,
    remote_root_locator: String,
    display_name: String,
    allow_relocation: bool,
) -> CommandResult<LibraryRegistrySnapshot> {
    let config = load_app_config(app_data_dir)?;
    let existing = config
        .libraries
        .iter()
        .find(|entry| entry.id() == library_id)
        .cloned()
        .ok_or_else(|| library_error(format!("remote repository {library_id} was not found")))?;
    if !matches!(existing, RegisteredLibrary::Remote { .. }) {
        return Err(library_error(format!(
            "repository {library_id} is not a remote repository"
        )));
    }

    let mut sessions = state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?;
    let (account_id, provider, webdav, google_drive, dropbox) = {
        let session = sessions
            .get(&session_id)
            .ok_or_else(|| library_error(format!("remote auth session {session_id} was not found")))?;
        (
            session.account_id.clone(),
            session.provider,
            session.webdav.clone(),
            session.google_drive.clone(),
            session.dropbox.clone(),
        )
    };

    if existing.provider() != Some(provider) {
        return Err(library_error(
            "reauthorization provider does not match the remote repository".to_owned(),
        ));
    }
    if provider != RemoteLibraryProvider::WebDav && existing.account_id() != Some(account_id.as_str()) {
        return Err(library_error(
            "reauthorization account does not match the remote repository".to_owned(),
        ));
    }
    let is_relocation = existing.remote_root_locator() != Some(remote_root_locator.as_str());
    if is_relocation && !allow_relocation {
        return Err(library_error(
            "The selected location is different from the saved remote repository location."
                .to_owned(),
        ));
    }

    if let Some(session) = sessions.get_mut(&session_id) {
        session.state = RemoteAuthState::Ready;
        session.remote_root_locator = Some(remote_root_locator.clone());
        session.display_name = Some(display_name.clone());
    }
    drop(sessions);

    let root_path = existing
        .working_copy_root()
        .ok_or_else(|| library_error("remote repository is missing a local working copy"))?;
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
                        "missing Google Drive session details during reauthorization".to_owned(),
                    )
                })?,
        }),
        RemoteLibraryProvider::Dropbox => Some(RemoteLibraryConnectionConfig::Dropbox {
            app_key: dropbox
                .as_ref()
                .map(|session| session.app_key.clone())
                .ok_or_else(|| {
                    library_error("missing Dropbox session details during reauthorization".to_owned())
                })?,
        }),
        RemoteLibraryProvider::WebDav => Some(RemoteLibraryConnectionConfig::WebDav {
            server_url: webdav
                .as_ref()
                .map(|session| session.server_url.clone())
                .ok_or_else(|| {
                    library_error("missing WebDAV session details during reauthorization".to_owned())
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
        Some(root_path.join("openkara.db").display().to_string()),
        existing.remote_revision().map(str::to_owned),
    );

    let remote_revision = match provider {
        RemoteLibraryProvider::GoogleDrive => {
            let google = google_drive.clone().ok_or_else(|| {
                library_error("missing Google Drive session details during reauthorization".to_owned())
            })?;
            let access_token = google.access_token.clone().ok_or_else(|| {
                library_error(
                    "Google Drive sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let refresh_token = google.refresh_token.clone().ok_or_else(|| {
                library_error(
                    "Google Drive did not return a refresh token. Reauthorize and ensure consent was granted."
                        .to_owned(),
                )
            })?;
            let secret = GoogleDriveSecret {
                library_id: library_id.clone(),
                client_id: google.client_id,
                client_secret: google.client_secret,
                access_token,
                refresh_token,
                access_token_expires_at_ms: google.access_token_expires_at_ms,
            };
            refresh_existing_google_drive_library(app_data_dir, &provisional_library, &secret)?
        }
        RemoteLibraryProvider::Dropbox => {
            let dropbox = dropbox.clone().ok_or_else(|| {
                library_error("missing Dropbox session details during reauthorization".to_owned())
            })?;
            let access_token = dropbox.access_token.clone().ok_or_else(|| {
                library_error(
                    "Dropbox sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let refresh_token = dropbox.refresh_token.clone().ok_or_else(|| {
                library_error(
                    "Dropbox did not return a refresh token. Reauthorize and ensure consent was granted."
                        .to_owned(),
                )
            })?;
            let secret = DropboxSecret {
                library_id: library_id.clone(),
                app_key: dropbox.app_key,
                app_secret: dropbox.app_secret,
                access_token,
                refresh_token,
                access_token_expires_at_ms: dropbox.access_token_expires_at_ms,
            };
            refresh_existing_dropbox_library(app_data_dir, &provisional_library, &secret)?
        }
        RemoteLibraryProvider::WebDav => {
            let webdav = webdav.clone().ok_or_else(|| {
                library_error("missing WebDAV session details during reauthorization".to_owned())
            })?;
            let secret = WebDavSecret {
                root_url: remote_root_locator.clone(),
                username: webdav.username,
                password: webdav.password,
            };
            refresh_existing_webdav_library(app_data_dir, &provisional_library, &secret)?
        }
    };

    match provider {
        RemoteLibraryProvider::GoogleDrive => {
            let secret = google_drive.ok_or_else(|| {
                library_error("missing Google Drive session details during reauthorization".to_owned())
            })?;
            let access_token = secret.access_token.ok_or_else(|| {
                library_error(
                    "Google Drive sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let refresh_token = secret.refresh_token.ok_or_else(|| {
                library_error(
                    "Google Drive did not return a refresh token. Reauthorize and ensure consent was granted."
                        .to_owned(),
                )
            })?;
            store_google_drive_secret(
                app_data_dir,
                GoogleDriveSecret {
                    library_id: library_id.clone(),
                    client_id: secret.client_id,
                    client_secret: secret.client_secret,
                    access_token,
                    refresh_token,
                    access_token_expires_at_ms: secret.access_token_expires_at_ms,
                },
            )?;
        }
        RemoteLibraryProvider::Dropbox => {
            let secret = dropbox.ok_or_else(|| {
                library_error("missing Dropbox session details during reauthorization".to_owned())
            })?;
            let access_token = secret.access_token.ok_or_else(|| {
                library_error(
                    "Dropbox sign-in has not completed yet. Finish the browser flow first."
                        .to_owned(),
                )
            })?;
            let refresh_token = secret.refresh_token.ok_or_else(|| {
                library_error(
                    "Dropbox did not return a refresh token. Reauthorize and ensure consent was granted."
                        .to_owned(),
                )
            })?;
            store_dropbox_secret(
                app_data_dir,
                DropboxSecret {
                    library_id: library_id.clone(),
                    app_key: secret.app_key,
                    app_secret: secret.app_secret,
                    access_token,
                    refresh_token,
                    access_token_expires_at_ms: secret.access_token_expires_at_ms,
                },
            )?;
        }
        RemoteLibraryProvider::WebDav => {
            let secret = webdav.ok_or_else(|| {
                library_error("missing WebDAV session details during reauthorization".to_owned())
            })?;
            store_webdav_secret(app_data_dir, &library_id, secret.username, secret.password)?;
        }
    }

    let mut config = load_app_config(app_data_dir)?;
    let updated_library = RegisteredLibrary::remote(
        library_id.clone(),
        display_name,
        provider,
        account_id,
        remote_root_locator,
        remote_path_display,
        connection_config,
        Some(root_path.join("openkara.db").display().to_string()),
        remote_revision.or_else(|| Some(current_unix_time_ms().to_string())),
    );
    if let Some(existing_entry) = config
        .libraries
        .iter_mut()
        .find(|entry| entry.id() == library_id)
    {
        *existing_entry = updated_library;
    }
    config.active_library_id = Some(library_id);
    persist_app_config(app_data_dir, &config)?;

    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(LibraryRoot::open(&root_path).map_err(library_error)?);

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
