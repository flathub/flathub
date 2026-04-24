use crate::{
    cache,
    commands::error::{library_error, CommandResult},
    config::RegisteredLibrary,
    library_root::LibraryRoot,
};
use reqwest::{blocking::Client, Method, Url};
use std::{
    collections::HashMap,
    fs,
    io::Write,
    net::{Ipv4Addr, SocketAddrV4, TcpListener},
    path::Path,
    sync::{Arc, Mutex},
    thread,
};
use tiny_http::Server;

use super::{
    auth::{
        env_optional, form_urlencoded_body, oauth_callback_response, oauth_pkce_code_challenge,
        random_token, remote_auth_session_exists, update_remote_auth_session,
    },
    types::{
        current_unix_time_ms, load_remote_credential, store_remote_credential,
        stored_google_drive_client_id, BundledGoogleDriveOAuthClientFile,
        GoogleDriveFileListResponse, GoogleDriveFileMetadata, GoogleDriveProviderCredentials,
        GoogleDriveSecret, GoogleDriveSessionData, GoogleDriveTokenResponse,
        GoogleDriveUserInfoResponse, RemoteAuthSession, RemoteAuthState,
        StoredGoogleDriveSecret, GOOGLE_DRIVE_CLIENT_ID_ENV, GOOGLE_DRIVE_CLIENT_SECRET_ENV,
        GOOGLE_DRIVE_OAUTH_CLIENT_RESOURCE_PATH, GOOGLE_DRIVE_OAUTH_SCOPE,
    },
};

const GOOGLE_DRIVE_FOLDER_MIME_TYPE: &str = "application/vnd.google-apps.folder";
pub(crate) const GOOGLE_DRIVE_ROOT_ID: &str = "root";

pub(crate) fn build_google_drive_authorization_url(
    session: &GoogleDriveSessionData,
) -> CommandResult<String> {
    let mut url = Url::parse("https://accounts.google.com/o/oauth2/v2/auth")
        .map_err(|error| library_error(format!("failed to build Google auth URL: {error}")))?;
    url.query_pairs_mut()
        .append_pair("client_id", &session.client_id)
        .append_pair("redirect_uri", &session.redirect_uri)
        .append_pair("response_type", "code")
        .append_pair("scope", GOOGLE_DRIVE_OAUTH_SCOPE)
        .append_pair("access_type", "offline")
        .append_pair("prompt", "consent")
        .append_pair("code_challenge", &oauth_pkce_code_challenge(&session.code_verifier))
        .append_pair("code_challenge_method", "S256")
        .append_pair("state", &session.state_token);
    Ok(url.to_string())
}

pub(crate) fn google_drive_provider_credentials_from_env(
    client_id: Option<String>,
    client_secret: Option<String>,
) -> CommandResult<GoogleDriveProviderCredentials> {
    let Some(client_id) = client_id.filter(|value| !value.trim().is_empty()) else {
        return Err(library_error(format!(
            "Google Drive is not available because the official app credential is missing. Set {GOOGLE_DRIVE_CLIENT_ID_ENV} before starting OpenKara."
        )));
    };

    Ok(GoogleDriveProviderCredentials {
        client_id,
        client_secret: client_secret.filter(|value| !value.trim().is_empty()),
    })
}

fn load_google_drive_provider_credentials_from_resource_dir(
    resource_dir: &Path,
) -> CommandResult<Option<GoogleDriveProviderCredentials>> {
    let path = resource_dir.join(GOOGLE_DRIVE_OAUTH_CLIENT_RESOURCE_PATH);
    if !path.exists() {
        return Ok(None);
    }

    let raw = fs::read_to_string(&path).map_err(|error| {
        library_error(format!(
            "failed to read bundled Google Drive OAuth client metadata at {}: {error}",
            path.display()
        ))
    })?;
    let bundled: BundledGoogleDriveOAuthClientFile =
        serde_json::from_str(&raw).map_err(|error| {
            library_error(format!(
                "failed to parse bundled Google Drive OAuth client metadata at {}: {error}",
                path.display()
            ))
        })?;

    Ok(Some(GoogleDriveProviderCredentials {
        client_id: bundled.installed.client_id,
        client_secret: bundled.installed.client_secret,
    }))
}

fn resolve_google_drive_provider_credentials(
    resource_dir: &Path,
) -> CommandResult<GoogleDriveProviderCredentials> {
    if let Some(credentials) = load_google_drive_provider_credentials_from_resource_dir(resource_dir)?
    {
        return Ok(credentials);
    }

    google_drive_provider_credentials_from_env(
        env_optional(GOOGLE_DRIVE_CLIENT_ID_ENV),
        env_optional(GOOGLE_DRIVE_CLIENT_SECRET_ENV),
    )
}

pub(crate) fn parse_google_drive_payload(
    resource_dir: &Path,
    _payload: Option<serde_json::Value>,
) -> CommandResult<GoogleDriveSessionData> {
    let credentials = resolve_google_drive_provider_credentials(resource_dir)?;

    Ok(GoogleDriveSessionData {
        client_id: credentials.client_id,
        client_secret: credentials.client_secret,
        code_verifier: random_token(64),
        redirect_uri: String::new(),
        state_token: random_token(48),
        root_folder_id: None,
        access_token: None,
        refresh_token: None,
        access_token_expires_at_ms: None,
    })
}

pub(crate) fn store_google_drive_secret(
    app_data_dir: &Path,
    secret: GoogleDriveSecret,
) -> CommandResult<()> {
    store_remote_credential(
        app_data_dir,
        &secret.library_id,
        &StoredGoogleDriveSecret {
            client_secret: secret.client_secret,
            access_token: secret.access_token,
            refresh_token: secret.refresh_token,
            access_token_expires_at_ms: secret.access_token_expires_at_ms,
        },
    )
}

pub(crate) fn load_google_drive_secret(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<GoogleDriveSecret> {
    if let Some(secret) =
        load_remote_credential::<StoredGoogleDriveSecret>(app_data_dir, library.id())?
    {
        let client_id = stored_google_drive_client_id(library)?;
        return Ok(GoogleDriveSecret {
            library_id: library.id().to_owned(),
            client_id,
            client_secret: secret.client_secret,
            access_token: secret.access_token,
            refresh_token: secret.refresh_token,
            access_token_expires_at_ms: secret.access_token_expires_at_ms,
        });
    }
    Err(library_error(
        "missing stored credentials for the remote library".to_owned(),
    ))
}

fn google_drive_api_url(path: &str) -> CommandResult<Url> {
    Url::parse(&format!("https://www.googleapis.com{path}"))
        .map_err(|error| library_error(format!("failed to build Google Drive URL: {error}")))
}

fn google_drive_refresh_access_token(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
) -> CommandResult<String> {
    if let Some(expires_at_ms) = secret.access_token_expires_at_ms {
        if expires_at_ms > current_unix_time_ms() + 60_000 && !secret.access_token.is_empty() {
            return Ok(secret.access_token.clone());
        }
    } else if !secret.access_token.is_empty() {
        return Ok(secret.access_token.clone());
    }

    let mut params = vec![
        ("client_id", secret.client_id.clone()),
        ("refresh_token", secret.refresh_token.clone()),
        ("grant_type", "refresh_token".to_owned()),
    ];
    if let Some(client_secret) = secret.client_secret.clone() {
        params.push(("client_secret", client_secret));
    }

    let body = form_urlencoded_body(&params)?;

    let response = Client::new()
        .post("https://oauth2.googleapis.com/token")
        .header("Content-Type", "application/x-www-form-urlencoded")
        .body(body)
        .send()
        .map_err(|error| library_error(format!("failed to refresh Google Drive access token: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive token refresh failed with status {}",
            response.status()
        )));
    }
    let body: GoogleDriveTokenResponse = response
        .json()
        .map_err(|error| library_error(format!("failed to parse Google Drive token response: {error}")))?;
    secret.access_token = body.access_token.clone();
    secret.access_token_expires_at_ms = body
        .expires_in
        .map(|seconds| current_unix_time_ms() + seconds * 1000);
    store_google_drive_secret(app_data_dir, secret.clone())?;
    Ok(secret.access_token.clone())
}

fn google_drive_authorized_request(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    method: Method,
    url: Url,
) -> CommandResult<reqwest::blocking::RequestBuilder> {
    let token = google_drive_refresh_access_token(app_data_dir, secret)?;
    Ok(Client::new().request(method, url).bearer_auth(token))
}

fn google_drive_request_with_access_token(
    access_token: &str,
    method: Method,
    url: Url,
) -> reqwest::blocking::RequestBuilder {
    Client::new().request(method, url).bearer_auth(access_token)
}

fn google_drive_escape_query_value(value: &str) -> String {
    value.replace('\\', "\\\\").replace('\'', "\\'")
}

fn google_drive_find_child(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    parent_id: &str,
    name: &str,
    mime_type: Option<&str>,
) -> CommandResult<Option<GoogleDriveFileMetadata>> {
    let mut url = google_drive_api_url("/drive/v3/files")?;
    let mut query = format!(
        "name = '{}' and '{}' in parents and trashed = false",
        google_drive_escape_query_value(name),
        google_drive_escape_query_value(parent_id)
    );
    if let Some(mime_type) = mime_type {
        query.push_str(&format!(
            " and mimeType = '{}'",
            google_drive_escape_query_value(mime_type)
        ));
    }
    url.query_pairs_mut()
        .append_pair("q", &query)
        .append_pair(
            "fields",
            "files(id,name,mimeType,headRevisionId,modifiedTime)",
        )
        .append_pair("spaces", "drive");

    let response = google_drive_authorized_request(app_data_dir, secret, Method::GET, url)?
        .send()
        .map_err(|error| library_error(format!("Google Drive lookup failed: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive lookup failed with status {}",
            response.status()
        )));
    }
    let body: GoogleDriveFileListResponse = response
        .json()
        .map_err(|error| library_error(format!("failed to parse Google Drive file list: {error}")))?;
    Ok(body.files.into_iter().next())
}

fn google_drive_find_child_with_token(
    access_token: &str,
    parent_id: &str,
    name: &str,
    mime_type: Option<&str>,
) -> CommandResult<Option<GoogleDriveFileMetadata>> {
    let mut url = google_drive_api_url("/drive/v3/files")?;
    let mut query = format!(
        "name = '{}' and '{}' in parents and trashed = false",
        google_drive_escape_query_value(name),
        google_drive_escape_query_value(parent_id)
    );
    if let Some(mime_type) = mime_type {
        query.push_str(&format!(
            " and mimeType = '{}'",
            google_drive_escape_query_value(mime_type)
        ));
    }
    url.query_pairs_mut()
        .append_pair("q", &query)
        .append_pair(
            "fields",
            "files(id,name,mimeType,headRevisionId,modifiedTime)",
        )
        .append_pair("spaces", "drive");

    let response = google_drive_request_with_access_token(access_token, Method::GET, url)
        .send()
        .map_err(|error| library_error(format!("Google Drive lookup failed: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive lookup failed with status {}",
            response.status()
        )));
    }
    let body: GoogleDriveFileListResponse = response
        .json()
        .map_err(|error| library_error(format!("failed to parse Google Drive file list: {error}")))?;
    Ok(body.files.into_iter().next())
}

fn google_drive_create_folder(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    parent_id: &str,
    name: &str,
) -> CommandResult<GoogleDriveFileMetadata> {
    let url = google_drive_api_url("/drive/v3/files?fields=id,name,mimeType")?;
    let response = google_drive_authorized_request(app_data_dir, secret, Method::POST, url)?
        .json(&serde_json::json!({
            "name": name,
            "mimeType": GOOGLE_DRIVE_FOLDER_MIME_TYPE,
            "parents": [parent_id],
        }))
        .send()
        .map_err(|error| library_error(format!("failed to create Google Drive folder: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive folder creation failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse folder creation response: {error}")))
}

fn google_drive_get_or_create_folder(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    parent_id: &str,
    name: &str,
) -> CommandResult<GoogleDriveFileMetadata> {
    if let Some(existing) = google_drive_find_child(
        app_data_dir,
        secret,
        parent_id,
        name,
        Some(GOOGLE_DRIVE_FOLDER_MIME_TYPE),
    )? {
        return Ok(existing);
    }
    google_drive_create_folder(app_data_dir, secret, parent_id, name)
}

fn google_drive_create_folder_with_token(
    access_token: &str,
    parent_id: &str,
    name: &str,
) -> CommandResult<GoogleDriveFileMetadata> {
    let url = google_drive_api_url("/drive/v3/files?fields=id,name,mimeType")?;
    let response = google_drive_request_with_access_token(access_token, Method::POST, url)
        .json(&serde_json::json!({
            "name": name,
            "mimeType": GOOGLE_DRIVE_FOLDER_MIME_TYPE,
            "parents": [parent_id],
        }))
        .send()
        .map_err(|error| library_error(format!("failed to create Google Drive folder: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive folder creation failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse folder creation response: {error}")))
}

pub(crate) fn google_drive_get_or_create_folder_with_token(
    access_token: &str,
    parent_id: &str,
    name: &str,
) -> CommandResult<GoogleDriveFileMetadata> {
    if let Some(existing) = google_drive_find_child_with_token(
        access_token,
        parent_id,
        name,
        Some(GOOGLE_DRIVE_FOLDER_MIME_TYPE),
    )? {
        return Ok(existing);
    }
    google_drive_create_folder_with_token(access_token, parent_id, name)
}

fn google_drive_create_empty_file(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    parent_id: &str,
    name: &str,
) -> CommandResult<GoogleDriveFileMetadata> {
    let url = google_drive_api_url(
        "/drive/v3/files?fields=id,name,mimeType,headRevisionId,modifiedTime",
    )?;
    let response = google_drive_authorized_request(app_data_dir, secret, Method::POST, url)?
        .json(&serde_json::json!({
            "name": name,
            "parents": [parent_id],
        }))
        .send()
        .map_err(|error| library_error(format!("failed to create Google Drive file metadata: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive file metadata creation failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse file creation response: {error}")))
}

fn google_drive_upload_file_bytes(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    file_id: &str,
    bytes: Vec<u8>,
) -> CommandResult<GoogleDriveFileMetadata> {
    let url = google_drive_api_url(&format!(
        "/upload/drive/v3/files/{file_id}?uploadType=media&fields=id,name,mimeType,headRevisionId,modifiedTime"
    ))?;
    let response = google_drive_authorized_request(app_data_dir, secret, Method::PATCH, url)?
        .header("Content-Type", "application/octet-stream")
        .body(bytes)
        .send()
        .map_err(|error| library_error(format!("failed to upload Google Drive file bytes: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive file upload failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse upload response: {error}")))
}

pub(crate) fn google_drive_download_file(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    file_id: &str,
    destination: &Path,
) -> CommandResult<()> {
    let url = google_drive_api_url(&format!("/drive/v3/files/{file_id}?alt=media"))?;
    let response = google_drive_authorized_request(app_data_dir, secret, Method::GET, url)?
        .send()
        .map_err(|error| library_error(format!("failed to download Google Drive file: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive download failed with status {}",
            response.status()
        )));
    }
    if let Some(parent) = destination.parent() {
        fs::create_dir_all(parent).map_err(|error| {
            library_error(format!("failed to create {}: {error}", parent.display()))
        })?;
    }
    let bytes = response
        .bytes()
        .map_err(|error| library_error(format!("failed to read Google Drive response: {error}")))?;
    let mut file = fs::File::create(destination).map_err(|error| {
        library_error(format!("failed to create {}: {error}", destination.display()))
    })?;
    file.write_all(bytes.as_ref()).map_err(|error| {
        library_error(format!("failed to write {}: {error}", destination.display()))
    })?;
    Ok(())
}

pub(crate) fn google_drive_root_display_name(display_name: &str) -> String {
    format!("My Drive/{display_name}")
}

fn google_drive_exchange_code_for_tokens(
    session: &GoogleDriveSessionData,
    code: &str,
) -> CommandResult<GoogleDriveTokenResponse> {
    let mut params = vec![
        ("client_id", session.client_id.clone()),
        ("code", code.to_owned()),
        ("code_verifier", session.code_verifier.clone()),
        ("grant_type", "authorization_code".to_owned()),
        ("redirect_uri", session.redirect_uri.clone()),
    ];
    if let Some(client_secret) = session.client_secret.clone() {
        params.push(("client_secret", client_secret));
    }

    let body = form_urlencoded_body(&params)?;

    let response = Client::new()
        .post("https://oauth2.googleapis.com/token")
        .header("Content-Type", "application/x-www-form-urlencoded")
        .body(body)
        .send()
        .map_err(|error| library_error(format!("failed to exchange Google auth code: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google auth code exchange failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse Google token response: {error}")))
}

fn google_drive_fetch_account_id(access_token: &str) -> CommandResult<String> {
    let url = Url::parse("https://openidconnect.googleapis.com/v1/userinfo")
        .map_err(|error| library_error(format!("failed to build Google userinfo URL: {error}")))?;
    let response = Client::new()
        .get(url)
        .bearer_auth(access_token)
        .send()
        .map_err(|error| library_error(format!("failed to fetch Google Drive account info: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive account lookup failed with status {}",
            response.status()
        )));
    }
    let body: GoogleDriveUserInfoResponse = response
        .json()
        .map_err(|error| library_error(format!("failed to parse Google Drive account info: {error}")))?;
    Ok(body.email.unwrap_or(body.sub))
}

pub(crate) fn spawn_google_drive_auth_worker(
    sessions: Arc<Mutex<HashMap<String, RemoteAuthSession>>>,
    session_id: String,
    session: GoogleDriveSessionData,
) -> CommandResult<GoogleDriveSessionData> {
    let listener = TcpListener::bind(SocketAddrV4::new(Ipv4Addr::LOCALHOST, 0))
        .map_err(|error| {
            library_error(format!("failed to bind Google OAuth loopback listener: {error}"))
        })?;
    let port = listener
        .local_addr()
        .map_err(|error| library_error(format!("failed to read Google OAuth listener address: {error}")))?
        .port();
    let server = Server::from_listener(listener, None)
        .map_err(|error| library_error(format!("failed to start Google OAuth listener: {error}")))?;

    let mut session = session;
    session.redirect_uri = format!("http://127.0.0.1:{port}/oauth2/callback");
    let worker_session = session.clone();

    thread::spawn(move || {
        let started_at = std::time::Instant::now();
        let request = loop {
            if !remote_auth_session_exists(&sessions, &session_id) {
                return;
            }

            match server.recv_timeout(std::time::Duration::from_secs(1)) {
                Ok(Some(request)) => break request,
                Ok(None) => {
                    if started_at.elapsed() >= std::time::Duration::from_secs(300) {
                        update_remote_auth_session(&sessions, &session_id, |state| {
                            state.state = RemoteAuthState::Failed;
                            state.error = Some(library_error(
                                "Google sign-in timed out before the browser returned to OpenKara."
                                    .to_owned(),
                            ));
                        });
                        return;
                    }
                }
                Err(error) => {
                    update_remote_auth_session(&sessions, &session_id, |state| {
                        state.state = RemoteAuthState::Failed;
                        state.error = Some(library_error(format!(
                            "Google sign-in listener failed: {error}"
                        )));
                    });
                    return;
                }
            }
        };

        let callback_url = format!("http://127.0.0.1:{port}{}", request.url());
        let parsed = match Url::parse(&callback_url) {
            Ok(parsed) => parsed,
            Err(error) => {
                let _ = request.respond(oauth_callback_response("Invalid OAuth callback."));
                update_remote_auth_session(&sessions, &session_id, |state| {
                    state.state = RemoteAuthState::Failed;
                    state.error = Some(library_error(format!(
                        "failed to parse Google OAuth callback: {error}"
                    )));
                });
                return;
            }
        };
        let query: HashMap<_, _> = parsed.query_pairs().into_owned().collect();

        if query.get("state") != Some(&worker_session.state_token) {
            let _ = request.respond(oauth_callback_response("OAuth state mismatch."));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(
                    "Google sign-in failed because the OAuth state token did not match."
                        .to_owned(),
                ));
            });
            return;
        }

        if let Some(error) = query.get("error") {
            let _ = request.respond(oauth_callback_response(
                "Google sign-in was cancelled or denied.",
            ));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(format!("Google sign-in failed: {error}")));
            });
            return;
        }

        let Some(code) = query.get("code") else {
            let _ = request.respond(oauth_callback_response("Missing Google authorization code."));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(
                    "Google sign-in did not return an authorization code.".to_owned(),
                ));
            });
            return;
        };

        match google_drive_exchange_code_for_tokens(&worker_session, code).and_then(|tokens| {
            let account_id = google_drive_fetch_account_id(&tokens.access_token)?;
            Ok((tokens, account_id))
        }) {
            Ok((tokens, account_id)) => {
                let _ = request.respond(oauth_callback_response(
                    "OpenKara connected to Google Drive. You can return to the app.",
                ));
                update_remote_auth_session(&sessions, &session_id, |state| {
                    state.state = RemoteAuthState::Ready;
                    state.account_id = account_id;
                    state.google_drive = Some(GoogleDriveSessionData {
                        access_token: Some(tokens.access_token.clone()),
                        refresh_token: tokens.refresh_token.clone(),
                        access_token_expires_at_ms: tokens
                            .expires_in
                            .map(|seconds| current_unix_time_ms() + seconds * 1000),
                        ..worker_session.clone()
                    });
                    state.error = None;
                });
            }
            Err(error) => {
                let _ = request.respond(oauth_callback_response(
                    "OpenKara could not finish Google Drive sign-in.",
                ));
                update_remote_auth_session(&sessions, &session_id, |state| {
                    state.state = RemoteAuthState::Failed;
                    state.error = Some(error);
                });
            }
        }
    });

    Ok(session)
}

pub(crate) fn google_drive_find_relative_entry(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    root_folder_id: &str,
    relative_path: &str,
) -> CommandResult<Option<GoogleDriveFileMetadata>> {
    let segments: Vec<&str> = relative_path
        .split('/')
        .filter(|segment| !segment.is_empty())
        .collect();
    if segments.is_empty() {
        return Ok(None);
    }

    let mut parent_id = root_folder_id.to_owned();
    for (index, segment) in segments.iter().enumerate() {
        let is_last = index == segments.len() - 1;
        let entry = google_drive_find_child(
            app_data_dir,
            secret,
            &parent_id,
            segment,
            if is_last {
                None
            } else {
                Some(GOOGLE_DRIVE_FOLDER_MIME_TYPE)
            },
        )?;
        let Some(entry) = entry else {
            return Ok(None);
        };
        if !is_last {
            parent_id = entry.id.clone();
        } else {
            return Ok(Some(entry));
        }
    }

    Ok(None)
}

pub(crate) fn google_drive_upload_relative_file_to_remote(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &GoogleDriveSecret,
    relative_path: &str,
    root_folder_id: &str,
) -> CommandResult<()> {
    let local_root = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let source = local_root.join(relative_path);
    let bytes = fs::read(&source)
        .map_err(|error| library_error(format!("failed to read {}: {error}", source.display())))?;
    let mut secret = secret.clone();

    let segments: Vec<&str> = relative_path
        .split('/')
        .filter(|segment| !segment.is_empty())
        .collect();
    if segments.is_empty() {
        return Ok(());
    }
    let file_name = segments.last().copied().unwrap_or_default();
    let mut parent_id = root_folder_id.to_owned();
    for segment in &segments[..segments.len() - 1] {
        let folder = google_drive_get_or_create_folder(app_data_dir, &mut secret, &parent_id, segment)?;
        parent_id = folder.id;
    }

    let file = match google_drive_find_child(app_data_dir, &mut secret, &parent_id, file_name, None)?
    {
        Some(file) => file,
        None => google_drive_create_empty_file(app_data_dir, &mut secret, &parent_id, file_name)?,
    };
    let _ = google_drive_upload_file_bytes(app_data_dir, &mut secret, &file.id, bytes)?;
    Ok(())
}

pub(crate) fn google_drive_upload_directory_to_remote(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &GoogleDriveSecret,
    relative_directory: &str,
    root_folder_id: &str,
) -> CommandResult<()> {
    let local_root = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let base = local_root.join(relative_directory);
    if !base.exists() {
        return Ok(());
    }
    for entry in fs::read_dir(&base)
        .map_err(|error| library_error(format!("failed to read {}: {error}", base.display())))?
    {
        let entry = entry.map_err(|error| library_error(error.to_string()))?;
        let path = entry.path();
        let relative = path
            .strip_prefix(&local_root)
            .map_err(|error| library_error(error.to_string()))?
            .to_string_lossy()
            .replace('\\', "/");
        if path.is_dir() {
            google_drive_upload_directory_to_remote(
                app_data_dir,
                library,
                secret,
                &relative,
                root_folder_id,
            )?;
        } else {
            google_drive_upload_relative_file_to_remote(
                app_data_dir,
                library,
                secret,
                &relative,
                root_folder_id,
            )?;
        }
    }
    Ok(())
}

pub(crate) fn initialize_or_sync_google_drive_library(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &GoogleDriveSecret,
) -> CommandResult<Option<String>> {
    let root_path = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let root = if root_path.join(".openkara-library").exists() {
        LibraryRoot::open(&root_path).map_err(library_error)?
    } else {
        LibraryRoot::create(&root_path).map_err(library_error)?
    };
    cache::initialize_library_database(&root.database_path()).map_err(library_error)?;

    let root_folder_id = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
    let mut secret = secret.clone();
    for directory in ["media", "media-g", "stems"] {
        let _ = google_drive_get_or_create_folder(app_data_dir, &mut secret, root_folder_id, directory)?;
    }

    if google_drive_find_relative_entry(app_data_dir, &mut secret, root_folder_id, ".openkara-library")?
        .is_none()
    {
        let marker_path = root.resolve(".openkara-library");
        fs::write(&marker_path, b"openkara remote library\n").map_err(|error| {
            library_error(format!("failed to write {}: {error}", marker_path.display()))
        })?;
        google_drive_upload_relative_file_to_remote(
            app_data_dir,
            library,
            &secret,
            ".openkara-library",
            root_folder_id,
        )?;
    }

    let database_entry =
        google_drive_find_relative_entry(app_data_dir, &mut secret, root_folder_id, "openkara.db")?;
    let revision = if let Some(database_entry) = database_entry {
        google_drive_download_file(
            app_data_dir,
            &mut secret,
            &database_entry.id,
            &root.database_path(),
        )?;
        database_entry.head_revision_id.or(database_entry.modified_time)
    } else {
        google_drive_upload_relative_file_to_remote(
            app_data_dir,
            library,
            &secret,
            "openkara.db",
            root_folder_id,
        )?;
        let uploaded = google_drive_find_relative_entry(
            app_data_dir,
            &mut secret,
            root_folder_id,
            "openkara.db",
        )?
        .ok_or_else(|| {
            library_error(
                "Google Drive database upload succeeded but the file was not found afterwards"
                    .to_owned(),
            )
        })?;
        uploaded.head_revision_id.or(uploaded.modified_time)
    };

    Ok(revision)
}

fn google_drive_delete_entry(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    file_id: &str,
) -> CommandResult<()> {
    let url = google_drive_api_url(&format!("/drive/v3/files/{file_id}"))?;
    let response = google_drive_authorized_request(app_data_dir, secret, Method::DELETE, url)?
        .send()
        .map_err(|error| library_error(format!("failed to delete Google Drive entry: {error}")))?;
    match response.status() {
        reqwest::StatusCode::NO_CONTENT | reqwest::StatusCode::NOT_FOUND => Ok(()),
        status => Err(library_error(format!(
            "Google Drive delete failed with status {status}"
        ))),
    }
}

pub(crate) fn delete_relative_path_from_remote(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    relative_path: &str,
) -> CommandResult<()> {
    let mut secret = load_google_drive_secret(app_data_dir, library)?;
    let root_folder_id = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
    let Some(entry) =
        google_drive_find_relative_entry(app_data_dir, &mut secret, root_folder_id, relative_path)?
    else {
        return Ok(());
    };
    google_drive_delete_entry(app_data_dir, &mut secret, &entry.id)
}

pub(crate) fn delete_remote_root(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<()> {
    let mut secret = load_google_drive_secret(app_data_dir, library)?;
    let root_folder_id = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
    google_drive_delete_entry(app_data_dir, &mut secret, root_folder_id)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::commands::remote_library::types::BundledGoogleDriveInstalledClient;
    use tempfile::tempdir;

    #[test]
    fn resolves_google_drive_credentials_from_non_ui_source() {
        let credentials = google_drive_provider_credentials_from_env(
            Some("client-123.apps.googleusercontent.com".to_owned()),
            Some("secret-456".to_owned()),
        )
        .expect("credentials should resolve");
        assert_eq!(credentials.client_id, "client-123.apps.googleusercontent.com");
        assert_eq!(credentials.client_secret.as_deref(), Some("secret-456"));
    }

    #[test]
    fn resolves_google_drive_credentials_from_bundled_resource_before_env() {
        let temp_dir = tempdir().expect("temp dir should create");
        let oauth_dir = temp_dir.path().join("oauth");
        fs::create_dir_all(&oauth_dir).expect("oauth directory should create");
        fs::write(
            oauth_dir.join("google-drive-client.json"),
            serde_json::to_vec(&BundledGoogleDriveOAuthClientFile {
                installed: BundledGoogleDriveInstalledClient {
                    client_id: "stored-client.apps.googleusercontent.com".to_owned(),
                    client_secret: Some("stored-secret".to_owned()),
                },
            })
            .expect("oauth file should serialize"),
        )
        .expect("oauth file should write");

        let credentials = resolve_google_drive_provider_credentials(temp_dir.path())
            .expect("credentials should resolve");
        assert_eq!(credentials.client_id, "stored-client.apps.googleusercontent.com");
        assert_eq!(credentials.client_secret.as_deref(), Some("stored-secret"));
    }

    #[test]
    fn google_drive_credentials_require_a_client_id() {
        let error = google_drive_provider_credentials_from_env(None, None)
            .expect_err("missing client id should fail");
        assert!(error.message.contains(GOOGLE_DRIVE_CLIENT_ID_ENV));
    }

    #[test]
    fn google_drive_auth_url_uses_loopback_pkce_and_offline_access() {
        let session = GoogleDriveSessionData {
            client_id: "client-123.apps.googleusercontent.com".to_owned(),
            client_secret: Some("secret-456".to_owned()),
            code_verifier: "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJK".to_owned(),
            redirect_uri: "http://127.0.0.1:43123/oauth2/callback".to_owned(),
            state_token: "state-123".to_owned(),
            root_folder_id: None,
            access_token: None,
            refresh_token: None,
            access_token_expires_at_ms: None,
        };

        let url = build_google_drive_authorization_url(&session).expect("url should build");
        let parsed = Url::parse(&url).expect("auth url should parse");
        let query: std::collections::HashMap<_, _> = parsed.query_pairs().into_owned().collect();

        assert_eq!(parsed.host_str(), Some("accounts.google.com"));
        assert_eq!(query.get("client_id"), Some(&session.client_id));
        assert_eq!(query.get("response_type"), Some(&"code".to_owned()));
        assert_eq!(query.get("redirect_uri"), Some(&session.redirect_uri));
        assert_eq!(query.get("scope"), Some(&GOOGLE_DRIVE_OAUTH_SCOPE.to_owned()));
        assert_eq!(query.get("access_type"), Some(&"offline".to_owned()));
        assert_eq!(query.get("prompt"), Some(&"consent".to_owned()));
        assert_eq!(query.get("code_challenge_method"), Some(&"S256".to_owned()));
        assert_eq!(query.get("state"), Some(&session.state_token));
    }
}
