use crate::{
    commands::error::{library_error, state_lock_error, CommandResult},
    config::RemoteLibraryProvider,
    AppState,
};
use base64::{engine::general_purpose::URL_SAFE_NO_PAD, Engine as _};
use rand::distr::{Alphanumeric, SampleString};
use reqwest::{Method, StatusCode, Url};
use sha2::Digest;
use std::{
    collections::HashMap,
    env,
    sync::{Arc, Mutex},
};
use tiny_http::Response as TinyHttpResponse;

use super::{
    dropbox, google_drive,
    types::{
        current_unix_time_ms, session_id_for_provider, RemoteAuthSession, RemoteAuthStart,
        RemoteAuthState, RemoteAuthStatus,
    },
    webdav,
};

pub(crate) fn begin_remote_auth(
    state: &AppState,
    provider: RemoteLibraryProvider,
    payload: Option<serde_json::Value>,
) -> CommandResult<RemoteAuthStart> {
    let session_id = session_id_for_provider(provider);
    let mut google_drive_session = None;
    let mut dropbox_session = None;
    let webdav_session = match provider {
        RemoteLibraryProvider::GoogleDrive => {
            let google = google_drive::parse_google_drive_payload(&state.app_resource_dir, payload)?;
            google_drive_session = Some(google_drive::spawn_google_drive_auth_worker(
                Arc::clone(&state.remote_auth_sessions),
                session_id.clone(),
                google,
            )?);
            None
        }
        RemoteLibraryProvider::Dropbox => {
            let dropbox = dropbox::parse_dropbox_payload(&state.app_resource_dir, payload)?;
            dropbox_session = Some(dropbox::spawn_dropbox_auth_worker(
                Arc::clone(&state.remote_auth_sessions),
                session_id.clone(),
                dropbox,
            )?);
            None
        }
        RemoteLibraryProvider::WebDav => {
            let session = webdav::parse_webdav_payload(payload)?;
            let client = webdav::webdav_client()?;
            let response = webdav::webdav_send(
                &client,
                Method::HEAD,
                &session.server_url,
                &session.username,
                &session.password,
                None,
                None,
            )?;
            match response.status() {
                StatusCode::OK
                | StatusCode::NO_CONTENT
                | StatusCode::METHOD_NOT_ALLOWED
                | StatusCode::FOUND
                | StatusCode::MOVED_PERMANENTLY => Some(session),
                StatusCode::UNAUTHORIZED | StatusCode::FORBIDDEN => {
                    return Err(library_error(
                        "WebDAV authentication failed. Double-check the server URL, username, and password."
                            .to_owned(),
                    ))
                }
                status => {
                    return Err(library_error(format!(
                        "WebDAV server check failed with status {status}"
                    )))
                }
            }
        }
    };

    if provider == RemoteLibraryProvider::GoogleDrive {
        let google = google_drive_session
            .clone()
            .ok_or_else(|| library_error("missing Google Drive session state".to_owned()))?;
        let session = RemoteAuthSession {
            provider,
            state: RemoteAuthState::Pending,
            remote_root_locator: None,
            display_name: None,
            account_id: session_id.clone(),
            error: None,
            google_drive: Some(google.clone()),
            dropbox: None,
            webdav: None,
        };
        state
            .remote_auth_sessions
            .lock()
            .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?
            .insert(session_id.clone(), session);

        return Ok(RemoteAuthStart {
            session_id,
            provider,
            authorization_url: Some(google_drive::build_google_drive_authorization_url(&google)?),
            expires_at_ms: Some(current_unix_time_ms() + 15 * 60 * 1000),
        });
    }

    if provider == RemoteLibraryProvider::Dropbox {
        let dropbox = dropbox_session
            .clone()
            .ok_or_else(|| library_error("missing Dropbox session state".to_owned()))?;
        let session = RemoteAuthSession {
            provider,
            state: RemoteAuthState::Pending,
            remote_root_locator: None,
            display_name: None,
            account_id: session_id.clone(),
            error: None,
            google_drive: None,
            dropbox: Some(dropbox.clone()),
            webdav: None,
        };
        state
            .remote_auth_sessions
            .lock()
            .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?
            .insert(session_id.clone(), session);

        return Ok(RemoteAuthStart {
            session_id,
            provider,
            authorization_url: Some(dropbox::build_dropbox_authorization_url(&dropbox)?),
            expires_at_ms: Some(current_unix_time_ms() + 15 * 60 * 1000),
        });
    }

    let session = RemoteAuthSession {
        provider,
        state: RemoteAuthState::Pending,
        remote_root_locator: None,
        display_name: None,
        account_id: if let Some(webdav) = &webdav_session {
            format!(
                "{}@{}",
                webdav.username,
                Url::parse(&webdav.server_url)
                    .ok()
                    .and_then(|url| url.host_str().map(str::to_owned))
                    .unwrap_or_else(|| "webdav".to_owned())
            )
        } else {
            session_id.clone()
        },
        error: None,
        google_drive: None,
        dropbox: None,
        webdav: webdav_session,
    };

    state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?
        .insert(session_id.clone(), session);

    Ok(RemoteAuthStart {
        session_id,
        provider,
        authorization_url: None,
        expires_at_ms: Some(current_unix_time_ms() + 15 * 60 * 1000),
    })
}

pub(crate) fn poll_remote_auth(
    state: &AppState,
    session_id: String,
) -> CommandResult<RemoteAuthStatus> {
    let sessions = state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?;
    let session = sessions
        .get(&session_id)
        .ok_or_else(|| library_error(format!("remote auth session {session_id} was not found")))?;

    Ok(RemoteAuthStatus {
        session_id,
        provider: session.provider,
        state: session.state.clone(),
        remote_root_locator: session.remote_root_locator.clone(),
        display_name: session.display_name.clone(),
        error: session.error.clone(),
    })
}

pub(crate) fn cancel_remote_auth(state: &AppState, session_id: String) -> CommandResult<()> {
    state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?
        .remove(&session_id);
    Ok(())
}

pub(crate) fn open_external_url(url: String) -> CommandResult<()> {
    open::that_detached(url.clone())
        .map_err(|error| library_error(format!("failed to open external URL {url}: {error}")))
}

pub(crate) fn random_token(length: usize) -> String {
    Alphanumeric.sample_string(&mut rand::rng(), length)
}

pub(crate) fn oauth_pkce_code_challenge(code_verifier: &str) -> String {
    let digest = sha2::Sha256::digest(code_verifier.as_bytes());
    URL_SAFE_NO_PAD.encode(digest)
}

pub(crate) fn form_urlencoded_body(params: &[(&str, String)]) -> CommandResult<String> {
    let mut encoded = Url::parse("https://example.invalid")
        .map_err(|error| library_error(format!("failed to build token body: {error}")))?;
    {
        let mut pairs = encoded.query_pairs_mut();
        for (key, value) in params {
            pairs.append_pair(key, value);
        }
    }
    Ok(encoded.query().unwrap_or_default().to_owned())
}

pub(crate) fn oauth_callback_response(
    body: &str,
) -> TinyHttpResponse<std::io::Cursor<Vec<u8>>> {
    TinyHttpResponse::from_string(body.to_owned())
}

pub(crate) fn env_optional(name: &str) -> Option<String> {
    env::var(name)
        .ok()
        .map(|value| value.trim().to_owned())
        .filter(|value| !value.is_empty())
}

pub(crate) fn update_remote_auth_session(
    sessions: &Arc<Mutex<HashMap<String, RemoteAuthSession>>>,
    session_id: &str,
    update: impl FnOnce(&mut RemoteAuthSession),
) {
    if let Ok(mut guard) = sessions.lock() {
        if let Some(session) = guard.get_mut(session_id) {
            update(session);
        }
    }
}

pub(crate) fn remote_auth_session_exists(
    sessions: &Arc<Mutex<HashMap<String, RemoteAuthSession>>>,
    session_id: &str,
) -> bool {
    sessions
        .lock()
        .ok()
        .map(|guard| guard.contains_key(session_id))
        .unwrap_or(false)
}
