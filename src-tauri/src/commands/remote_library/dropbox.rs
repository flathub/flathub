use crate::{
    cache,
    commands::error::{library_error, CommandResult},
    config::RegisteredLibrary,
    library_root::LibraryRoot,
};
use reqwest::{Method, StatusCode, Url};
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
        current_unix_time_ms, load_remote_credential, slugify_display_name,
        store_remote_credential, stored_dropbox_app_key, BundledDropboxOAuthClientFile,
        DropboxAccountResponse, DropboxCreateFolderResponse, DropboxMetadata,
        DropboxProviderCredentials, DropboxSecret, DropboxSessionData, DropboxTokenResponse,
        RemoteAuthSession, RemoteAuthState, StoredDropboxSecret, DROPBOX_APP_KEY_ENV,
        DROPBOX_APP_SECRET_ENV, DROPBOX_FIXED_REDIRECT_PORT, DROPBOX_FIXED_REDIRECT_URI,
        DROPBOX_OAUTH_CLIENT_RESOURCE_PATH,
    },
};

pub(crate) fn build_dropbox_authorization_url(
    session: &DropboxSessionData,
) -> CommandResult<String> {
    let mut url = Url::parse("https://www.dropbox.com/oauth2/authorize")
        .map_err(|error| library_error(format!("failed to build Dropbox auth URL: {error}")))?;
    url.query_pairs_mut()
        .append_pair("client_id", &session.app_key)
        .append_pair("redirect_uri", &session.redirect_uri)
        .append_pair("response_type", "code")
        .append_pair("token_access_type", "offline")
        .append_pair("code_challenge", &oauth_pkce_code_challenge(&session.code_verifier))
        .append_pair("code_challenge_method", "S256")
        .append_pair("state", &session.state_token);
    Ok(url.to_string())
}

pub(crate) fn dropbox_provider_credentials_from_env(
    app_key: Option<String>,
    app_secret: Option<String>,
) -> CommandResult<DropboxProviderCredentials> {
    let Some(app_key) = app_key.filter(|value| !value.trim().is_empty()) else {
        return Err(library_error(format!(
            "Dropbox is not available because the official app credential is missing. Set {DROPBOX_APP_KEY_ENV} before starting OpenKara."
        )));
    };

    Ok(DropboxProviderCredentials {
        app_key,
        app_secret: app_secret.filter(|value| !value.trim().is_empty()),
    })
}

fn load_dropbox_provider_credentials_from_resource_dir(
    resource_dir: &Path,
) -> CommandResult<Option<DropboxProviderCredentials>> {
    let path = resource_dir.join(DROPBOX_OAUTH_CLIENT_RESOURCE_PATH);
    if !path.exists() {
        return Ok(None);
    }

    let raw = fs::read_to_string(&path).map_err(|error| {
        library_error(format!(
            "failed to read bundled Dropbox OAuth client metadata at {}: {error}",
            path.display()
        ))
    })?;
    let bundled: BundledDropboxOAuthClientFile =
        serde_json::from_str(&raw).map_err(|error| {
            library_error(format!(
                "failed to parse bundled Dropbox OAuth client metadata at {}: {error}",
                path.display()
            ))
        })?;

    dropbox_provider_credentials_from_env(Some(bundled.app_key), bundled.app_secret).map(Some)
}

fn resolve_dropbox_provider_credentials(resource_dir: &Path) -> CommandResult<DropboxProviderCredentials> {
    if let Some(credentials) = load_dropbox_provider_credentials_from_resource_dir(resource_dir)? {
        return Ok(credentials);
    }

    dropbox_provider_credentials_from_env(
        env_optional(DROPBOX_APP_KEY_ENV),
        env_optional(DROPBOX_APP_SECRET_ENV),
    )
}

pub(crate) fn parse_dropbox_payload(
    resource_dir: &Path,
    _payload: Option<serde_json::Value>,
) -> CommandResult<DropboxSessionData> {
    let credentials = resolve_dropbox_provider_credentials(resource_dir)?;

    Ok(DropboxSessionData {
        app_key: credentials.app_key,
        app_secret: credentials.app_secret,
        code_verifier: random_token(64),
        redirect_uri: String::new(),
        state_token: random_token(48),
        access_token: None,
        refresh_token: None,
        access_token_expires_at_ms: None,
    })
}

pub(crate) fn store_dropbox_secret(
    app_data_dir: &Path,
    secret: DropboxSecret,
) -> CommandResult<()> {
    store_remote_credential(
        app_data_dir,
        &secret.library_id,
        &StoredDropboxSecret {
            refresh_token: secret.refresh_token,
            access_token: secret.access_token,
            access_token_expires_at_ms: secret.access_token_expires_at_ms,
            app_secret: secret.app_secret,
        },
    )
}

pub(crate) fn load_dropbox_secret(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<DropboxSecret> {
    if let Some(secret) = load_remote_credential::<StoredDropboxSecret>(app_data_dir, library.id())?
    {
        let app_key = stored_dropbox_app_key(library)?;
        return Ok(DropboxSecret {
            library_id: library.id().to_owned(),
            app_key,
            app_secret: secret.app_secret,
            access_token: secret.access_token,
            refresh_token: secret.refresh_token,
            access_token_expires_at_ms: secret.access_token_expires_at_ms,
        });
    }
    Err(library_error(
        "missing stored credentials for the remote library".to_owned(),
    ))
}

fn dropbox_api_url(path: &str) -> CommandResult<Url> {
    Url::parse(&format!("https://api.dropboxapi.com{path}"))
        .map_err(|error| library_error(format!("failed to build Dropbox URL: {error}")))
}

fn dropbox_content_url(path: &str) -> CommandResult<Url> {
    Url::parse(&format!("https://content.dropboxapi.com{path}"))
        .map_err(|error| library_error(format!("failed to build Dropbox content URL: {error}")))
}

fn dropbox_refresh_access_token(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
) -> CommandResult<String> {
    if let Some(expires_at_ms) = secret.access_token_expires_at_ms {
        if expires_at_ms > current_unix_time_ms() + 60_000 && !secret.access_token.is_empty() {
            return Ok(secret.access_token.clone());
        }
    } else if !secret.access_token.is_empty() {
        return Ok(secret.access_token.clone());
    }

    let mut params = vec![
        ("client_id", secret.app_key.clone()),
        ("refresh_token", secret.refresh_token.clone()),
        ("grant_type", "refresh_token".to_owned()),
    ];
    if let Some(app_secret) = secret.app_secret.clone() {
        params.push(("client_secret", app_secret));
    }

    let body = form_urlencoded_body(&params)?;

    let response = reqwest::blocking::Client::new()
        .post("https://api.dropboxapi.com/oauth2/token")
        .header("Content-Type", "application/x-www-form-urlencoded")
        .body(body)
        .send()
        .map_err(|error| library_error(format!("failed to refresh Dropbox access token: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox token refresh failed with status {}",
            response.status()
        )));
    }

    let body: DropboxTokenResponse = response
        .json()
        .map_err(|error| library_error(format!("failed to parse Dropbox token response: {error}")))?;
    secret.access_token = body.access_token;
    secret.access_token_expires_at_ms = body
        .expires_in
        .map(|seconds| current_unix_time_ms() + seconds * 1000);
    store_dropbox_secret(app_data_dir, secret.clone())?;
    Ok(secret.access_token.clone())
}

fn dropbox_authorized_request(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    method: Method,
    url: Url,
) -> CommandResult<reqwest::blocking::RequestBuilder> {
    let token = dropbox_refresh_access_token(app_data_dir, secret)?;
    Ok(reqwest::blocking::Client::new()
        .request(method, url)
        .bearer_auth(token))
}

fn dropbox_request_with_access_token(
    access_token: &str,
    method: Method,
    url: Url,
) -> reqwest::blocking::RequestBuilder {
    reqwest::blocking::Client::new()
        .request(method, url)
        .bearer_auth(access_token)
}

fn dropbox_exchange_code_for_tokens(
    session: &DropboxSessionData,
    code: &str,
) -> CommandResult<DropboxTokenResponse> {
    let mut params = vec![
        ("client_id", session.app_key.clone()),
        ("code", code.to_owned()),
        ("code_verifier", session.code_verifier.clone()),
        ("grant_type", "authorization_code".to_owned()),
        ("redirect_uri", session.redirect_uri.clone()),
    ];
    if let Some(app_secret) = session.app_secret.clone() {
        params.push(("client_secret", app_secret));
    }

    let body = form_urlencoded_body(&params)?;

    let response = reqwest::blocking::Client::new()
        .post("https://api.dropboxapi.com/oauth2/token")
        .header("Content-Type", "application/x-www-form-urlencoded")
        .body(body)
        .send()
        .map_err(|error| library_error(format!("failed to exchange Dropbox auth code: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox auth code exchange failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse Dropbox token response: {error}")))
}

fn dropbox_fetch_account_id(access_token: &str) -> CommandResult<String> {
    let url = dropbox_api_url("/2/users/get_current_account")?;
    let response = reqwest::blocking::Client::new()
        .post(url)
        .bearer_auth(access_token)
        .header("Content-Type", "application/json")
        .body("{}")
        .send()
        .map_err(|error| library_error(format!("failed to fetch Dropbox account info: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox account lookup failed with status {}",
            response.status()
        )));
    }
    let body: DropboxAccountResponse = response
        .json()
        .map_err(|error| library_error(format!("failed to parse Dropbox account info: {error}")))?;
    Ok(body.email.unwrap_or(body.account_id))
}

pub(crate) fn spawn_dropbox_auth_worker(
    sessions: Arc<Mutex<HashMap<String, RemoteAuthSession>>>,
    session_id: String,
    session: DropboxSessionData,
) -> CommandResult<DropboxSessionData> {
    let listener = TcpListener::bind(SocketAddrV4::new(
        Ipv4Addr::LOCALHOST,
        DROPBOX_FIXED_REDIRECT_PORT,
    ))
    .map_err(|error| {
        library_error(format!(
            "failed to bind Dropbox OAuth listener on {DROPBOX_FIXED_REDIRECT_URI}: {error}"
        ))
    })?;
    let server = Server::from_listener(listener, None)
        .map_err(|error| library_error(format!("failed to start Dropbox OAuth listener: {error}")))?;

    let mut session = session;
    session.redirect_uri = DROPBOX_FIXED_REDIRECT_URI.to_owned();
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
                                "Dropbox sign-in timed out before the browser returned to OpenKara."
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
                            "Dropbox sign-in listener failed: {error}"
                        )));
                    });
                    return;
                }
            }
        };

        let callback_url =
            format!("http://127.0.0.1:{}{}", DROPBOX_FIXED_REDIRECT_PORT, request.url());
        let parsed = match Url::parse(&callback_url) {
            Ok(parsed) => parsed,
            Err(error) => {
                let _ = request.respond(oauth_callback_response("Invalid OAuth callback."));
                update_remote_auth_session(&sessions, &session_id, |state| {
                    state.state = RemoteAuthState::Failed;
                    state.error = Some(library_error(format!(
                        "failed to parse Dropbox OAuth callback: {error}"
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
                    "Dropbox sign-in failed because the OAuth state token did not match."
                        .to_owned(),
                ));
            });
            return;
        }

        if let Some(error) = query.get("error") {
            let _ = request.respond(oauth_callback_response(
                "Dropbox sign-in was cancelled or denied.",
            ));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(format!("Dropbox sign-in failed: {error}")));
            });
            return;
        }

        let Some(code) = query.get("code") else {
            let _ = request.respond(oauth_callback_response("Missing Dropbox authorization code."));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(
                    "Dropbox sign-in did not return an authorization code.".to_owned(),
                ));
            });
            return;
        };

        match dropbox_exchange_code_for_tokens(&worker_session, code).and_then(|tokens| {
            let account_id = dropbox_fetch_account_id(&tokens.access_token)?;
            Ok((tokens, account_id))
        }) {
            Ok((tokens, account_id)) => {
                let _ = request.respond(oauth_callback_response(
                    "OpenKara connected to Dropbox. You can return to the app.",
                ));
                update_remote_auth_session(&sessions, &session_id, |state| {
                    state.state = RemoteAuthState::Ready;
                    state.account_id = account_id;
                    state.dropbox = Some(DropboxSessionData {
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
                    "OpenKara could not finish Dropbox sign-in.",
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

pub(crate) fn normalize_dropbox_root_path(raw: Option<&str>, fallback_display_name: &str) -> String {
    let candidate = raw.unwrap_or_default().trim().trim_matches('/');
    let value = if candidate.is_empty() {
        slugify_display_name(fallback_display_name)
    } else {
        candidate.to_owned()
    };
    format!("/{}", value)
}

pub(crate) fn dropbox_join_path(root_path: &str, relative_path: &str) -> String {
    let relative = relative_path.trim_matches('/');
    if relative.is_empty() {
        root_path.to_owned()
    } else {
        format!("{}/{}", root_path.trim_end_matches('/'), relative)
    }
}

pub(crate) fn dropbox_metadata_revision(metadata: &DropboxMetadata) -> Option<String> {
    metadata.rev.clone().or(metadata.server_modified.clone())
}

pub(crate) fn dropbox_get_metadata(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    path: &str,
) -> CommandResult<Option<DropboxMetadata>> {
    let url = dropbox_api_url("/2/files/get_metadata")?;
    let response = dropbox_authorized_request(app_data_dir, secret, Method::POST, url)?
        .json(&serde_json::json!({ "path": path }))
        .send()
        .map_err(|error| library_error(format!("Dropbox metadata lookup failed: {error}")))?;
    match response.status() {
        StatusCode::OK => response
            .json()
            .map(Some)
            .map_err(|error| library_error(format!("failed to parse Dropbox metadata: {error}"))),
        StatusCode::CONFLICT => Ok(None),
        status => Err(library_error(format!(
            "Dropbox metadata lookup failed with status {}",
            status
        ))),
    }
}

fn dropbox_get_metadata_with_token(
    access_token: &str,
    path: &str,
) -> CommandResult<Option<DropboxMetadata>> {
    let url = dropbox_api_url("/2/files/get_metadata")?;
    let response = dropbox_request_with_access_token(access_token, Method::POST, url)
        .json(&serde_json::json!({ "path": path }))
        .send()
        .map_err(|error| library_error(format!("Dropbox metadata lookup failed: {error}")))?;
    match response.status() {
        StatusCode::OK => response
            .json()
            .map(Some)
            .map_err(|error| library_error(format!("failed to parse Dropbox metadata: {error}"))),
        StatusCode::CONFLICT => Ok(None),
        status => Err(library_error(format!(
            "Dropbox metadata lookup failed with status {}",
            status
        ))),
    }
}

fn dropbox_create_folder(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    path: &str,
) -> CommandResult<DropboxMetadata> {
    let url = dropbox_api_url("/2/files/create_folder_v2")?;
    let response = dropbox_authorized_request(app_data_dir, secret, Method::POST, url)?
        .json(&serde_json::json!({ "path": path, "autorename": false }))
        .send()
        .map_err(|error| library_error(format!("failed to create Dropbox folder: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox folder creation failed with status {}",
            response.status()
        )));
    }
    response
        .json::<DropboxCreateFolderResponse>()
        .map(|body| body.metadata)
        .map_err(|error| library_error(format!("failed to parse Dropbox folder creation response: {error}")))
}

fn dropbox_create_folder_with_token(
    access_token: &str,
    path: &str,
) -> CommandResult<DropboxMetadata> {
    let url = dropbox_api_url("/2/files/create_folder_v2")?;
    let response = dropbox_request_with_access_token(access_token, Method::POST, url)
        .json(&serde_json::json!({ "path": path, "autorename": false }))
        .send()
        .map_err(|error| library_error(format!("failed to create Dropbox folder: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox folder creation failed with status {}",
            response.status()
        )));
    }
    response
        .json::<DropboxCreateFolderResponse>()
        .map(|body| body.metadata)
        .map_err(|error| library_error(format!("failed to parse Dropbox folder creation response: {error}")))
}

fn dropbox_ensure_folder(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    path: &str,
) -> CommandResult<()> {
    let mut current = String::new();
    for segment in path.trim_matches('/').split('/').filter(|segment| !segment.is_empty()) {
        current.push('/');
        current.push_str(segment);
        if dropbox_get_metadata(app_data_dir, secret, &current)?.is_none() {
            let _ = dropbox_create_folder(app_data_dir, secret, &current)?;
        }
    }
    Ok(())
}

pub(crate) fn dropbox_ensure_folder_with_token(
    access_token: &str,
    path: &str,
) -> CommandResult<()> {
    let mut current = String::new();
    for segment in path.trim_matches('/').split('/').filter(|segment| !segment.is_empty()) {
        current.push('/');
        current.push_str(segment);
        if dropbox_get_metadata_with_token(access_token, &current)?.is_none() {
            let _ = dropbox_create_folder_with_token(access_token, &current)?;
        }
    }
    Ok(())
}

fn dropbox_upload_file_bytes(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    path: &str,
    bytes: Vec<u8>,
) -> CommandResult<DropboxMetadata> {
    let url = dropbox_content_url("/2/files/upload")?;
    let response = dropbox_authorized_request(app_data_dir, secret, Method::POST, url)?
        .header(
            "Dropbox-API-Arg",
            serde_json::json!({
                "path": path,
                "mode": "overwrite",
                "autorename": false,
                "mute": true,
                "strict_conflict": false
            })
            .to_string(),
        )
        .header("Content-Type", "application/octet-stream")
        .body(bytes)
        .send()
        .map_err(|error| library_error(format!("failed to upload Dropbox file bytes: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox file upload failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse Dropbox upload response: {error}")))
}

pub(crate) fn dropbox_download_file(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    path: &str,
    destination: &Path,
) -> CommandResult<()> {
    let url = dropbox_content_url("/2/files/download")?;
    let response = dropbox_authorized_request(app_data_dir, secret, Method::POST, url)?
        .header("Dropbox-API-Arg", serde_json::json!({ "path": path }).to_string())
        .send()
        .map_err(|error| library_error(format!("failed to download Dropbox file: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Dropbox download failed with status {}",
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
        .map_err(|error| library_error(format!("failed to read Dropbox response: {error}")))?;
    let mut file = fs::File::create(destination).map_err(|error| {
        library_error(format!("failed to create {}: {error}", destination.display()))
    })?;
    file.write_all(bytes.as_ref()).map_err(|error| {
        library_error(format!("failed to write {}: {error}", destination.display()))
    })?;
    Ok(())
}

pub(crate) fn dropbox_upload_relative_file_to_remote(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &DropboxSecret,
    relative_path: &str,
    root_path: &str,
) -> CommandResult<()> {
    let local_root = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let source = local_root.join(relative_path);
    let bytes = fs::read(&source)
        .map_err(|error| library_error(format!("failed to read {}: {error}", source.display())))?;
    let mut secret = secret.clone();
    if let Some(parent) = Path::new(relative_path).parent() {
        let parent_path = parent.to_string_lossy().replace('\\', "/");
        if !parent_path.is_empty() {
            dropbox_ensure_folder(
                app_data_dir,
                &mut secret,
                &dropbox_join_path(root_path, &parent_path),
            )?;
        }
    }
    let remote_path = dropbox_join_path(root_path, relative_path);
    let _ = dropbox_upload_file_bytes(app_data_dir, &mut secret, &remote_path, bytes)?;
    Ok(())
}

pub(crate) fn dropbox_upload_directory_to_remote(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &DropboxSecret,
    relative_directory: &str,
    root_path: &str,
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
            dropbox_upload_directory_to_remote(app_data_dir, library, secret, &relative, root_path)?;
        } else {
            dropbox_upload_relative_file_to_remote(app_data_dir, library, secret, &relative, root_path)?;
        }
    }
    Ok(())
}

pub(crate) fn initialize_or_sync_dropbox_library(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &DropboxSecret,
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

    let remote_root_path = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
    let mut secret = secret.clone();
    dropbox_ensure_folder(app_data_dir, &mut secret, remote_root_path)?;
    for directory in ["media", "media-g", "stems"] {
        dropbox_ensure_folder(
            app_data_dir,
            &mut secret,
            &dropbox_join_path(remote_root_path, directory),
        )?;
    }

    let marker_remote_path = dropbox_join_path(remote_root_path, ".openkara-library");
    if dropbox_get_metadata(app_data_dir, &mut secret, &marker_remote_path)?.is_none() {
        let marker_path = root.resolve(".openkara-library");
        fs::write(&marker_path, b"openkara remote library\n").map_err(|error| {
            library_error(format!("failed to write {}: {error}", marker_path.display()))
        })?;
        dropbox_upload_relative_file_to_remote(
            app_data_dir,
            library,
            &secret,
            ".openkara-library",
            remote_root_path,
        )?;
    }

    let database_remote_path = dropbox_join_path(remote_root_path, "openkara.db");
    let revision = if let Some(metadata) =
        dropbox_get_metadata(app_data_dir, &mut secret, &database_remote_path)?
    {
        dropbox_download_file(
            app_data_dir,
            &mut secret,
            &database_remote_path,
            &root.database_path(),
        )?;
        dropbox_metadata_revision(&metadata)
    } else {
        dropbox_upload_relative_file_to_remote(
            app_data_dir,
            library,
            &secret,
            "openkara.db",
            remote_root_path,
        )?;
        let metadata = dropbox_get_metadata(app_data_dir, &mut secret, &database_remote_path)?
            .ok_or_else(|| {
                library_error(
                    "Dropbox database upload succeeded but the file was not found afterwards"
                        .to_owned(),
                )
            })?;
        dropbox_metadata_revision(&metadata)
    };

    Ok(revision)
}

fn dropbox_delete_path(
    app_data_dir: &Path,
    secret: &mut DropboxSecret,
    path: &str,
) -> CommandResult<()> {
    let url = dropbox_api_url("/2/files/delete_v2")?;
    let response = dropbox_authorized_request(app_data_dir, secret, Method::POST, url)?
        .json(&serde_json::json!({ "path": path }))
        .send()
        .map_err(|error| library_error(format!("failed to delete Dropbox path: {error}")))?;
    match response.status() {
        StatusCode::OK | StatusCode::CONFLICT => Ok(()),
        status => Err(library_error(format!(
            "Dropbox delete failed with status {status}"
        ))),
    }
}

pub(crate) fn delete_relative_path_from_remote(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    relative_path: &str,
) -> CommandResult<()> {
    let mut secret = load_dropbox_secret(app_data_dir, library)?;
    let root_path = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
    dropbox_delete_path(
        app_data_dir,
        &mut secret,
        &dropbox_join_path(root_path, relative_path),
    )
}

pub(crate) fn delete_remote_root(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<()> {
    delete_relative_path_from_remote(app_data_dir, library, "")
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn resolves_dropbox_credentials_from_non_ui_source() {
        let credentials = dropbox_provider_credentials_from_env(
            Some("dropbox-app-key".to_owned()),
            Some("dropbox-app-secret".to_owned()),
        )
        .expect("credentials should resolve");
        assert_eq!(credentials.app_key, "dropbox-app-key");
        assert_eq!(credentials.app_secret.as_deref(), Some("dropbox-app-secret"));
    }

    #[test]
    fn resolves_dropbox_credentials_from_bundled_resource_before_env() {
        let temp_dir = tempdir().expect("temp dir should create");
        let oauth_dir = temp_dir.path().join("oauth");
        fs::create_dir_all(&oauth_dir).expect("oauth directory should create");
        fs::write(
            oauth_dir.join("dropbox-client.json"),
            serde_json::to_vec(&BundledDropboxOAuthClientFile {
                app_key: "stored-dropbox-app-key".to_owned(),
                app_secret: Some("stored-dropbox-app-secret".to_owned()),
            })
            .expect("oauth file should serialize"),
        )
        .expect("oauth file should write");

        let credentials = resolve_dropbox_provider_credentials(temp_dir.path())
            .expect("credentials should resolve");
        assert_eq!(credentials.app_key, "stored-dropbox-app-key");
        assert_eq!(
            credentials.app_secret.as_deref(),
            Some("stored-dropbox-app-secret")
        );
    }

    #[test]
    fn dropbox_credentials_require_an_app_key() {
        let error = dropbox_provider_credentials_from_env(None, None)
            .expect_err("missing app key should fail");
        assert!(error.message.contains(DROPBOX_APP_KEY_ENV));
    }

    #[test]
    fn dropbox_auth_url_uses_loopback_pkce_and_offline_access() {
        let session = DropboxSessionData {
            app_key: "dropbox-app-key".to_owned(),
            app_secret: Some("dropbox-app-secret".to_owned()),
            code_verifier: "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJK".to_owned(),
            redirect_uri: DROPBOX_FIXED_REDIRECT_URI.to_owned(),
            state_token: "state-123".to_owned(),
            access_token: None,
            refresh_token: None,
            access_token_expires_at_ms: None,
        };

        let url = build_dropbox_authorization_url(&session).expect("url should build");
        let parsed = Url::parse(&url).expect("auth url should parse");
        let query: std::collections::HashMap<_, _> = parsed.query_pairs().into_owned().collect();

        assert_eq!(parsed.host_str(), Some("www.dropbox.com"));
        assert_eq!(query.get("client_id"), Some(&session.app_key));
        assert_eq!(query.get("response_type"), Some(&"code".to_owned()));
        assert_eq!(query.get("redirect_uri"), Some(&session.redirect_uri));
        assert_eq!(query.get("token_access_type"), Some(&"offline".to_owned()));
        assert_eq!(query.get("code_challenge_method"), Some(&"S256".to_owned()));
        assert_eq!(query.get("state"), Some(&session.state_token));
    }
}
