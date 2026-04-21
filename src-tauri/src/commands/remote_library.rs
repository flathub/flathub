use crate::{
    cache,
    commands::error::{
        database_error, library_error, state_lock_error, CommandError, CommandResult,
    },
    config::{self, AppConfig, RegisteredLibrary, RemoteLibraryProvider},
    library::Song,
    library_root::LibraryRoot,
    AppState,
};
use base64::{engine::general_purpose::URL_SAFE_NO_PAD, Engine as _};
use reqwest::{
    blocking::{Client, Response},
    header::{ETAG, HeaderMap},
    Method, StatusCode, Url,
};
use rand::{distr::Alphanumeric, Rng};
use serde::{Deserialize, Serialize};
use sha2::Digest;
use std::{
    collections::HashMap,
    fs,
    io::Write,
    net::{Ipv4Addr, SocketAddrV4, TcpListener},
    path::{Path, PathBuf},
    sync::{Arc, Mutex},
    thread,
    time::{SystemTime, UNIX_EPOCH},
};
use tiny_http::{Response as TinyHttpResponse, Server};
use tauri::{AppHandle, Emitter, Manager, State};

const REMOTE_LIBRARIES_DIR: &str = "remote-libraries";
const REMOTE_LIBRARY_SECRETS_FILENAME: &str = "remote-library-secrets.json";

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
enum RemoteSecretRecord {
    GoogleDrive {
        client_id: String,
        client_secret: Option<String>,
        access_token: String,
        refresh_token: String,
        access_token_expires_at_ms: Option<i64>,
    },
    WebDav {
        server_url: String,
        username: String,
        password: String,
    },
}

#[derive(Debug, Default, Serialize, Deserialize)]
struct RemoteSecretsFile {
    #[serde(default)]
    libraries: HashMap<String, RemoteSecretRecord>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
enum RemoteAuthPayloadInput {
    GoogleDrive {
        client_id: String,
        client_secret: Option<String>,
    },
    WebDav {
        server_url: String,
        username: String,
        password: String,
        root_path: Option<String>,
    },
}

#[derive(Debug, Clone)]
struct WebDavSessionData {
    server_url: String,
    username: String,
    password: String,
    root_path: Option<String>,
}

#[derive(Debug, Clone)]
struct GoogleDriveSessionData {
    client_id: String,
    client_secret: Option<String>,
    code_verifier: String,
    redirect_uri: String,
    state_token: String,
    root_folder_id: Option<String>,
    access_token: Option<String>,
    refresh_token: Option<String>,
    access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone)]
struct WebDavSecret {
    library_id: String,
    server_url: String,
    root_url: String,
    username: String,
    password: String,
}

#[derive(Debug, Clone)]
struct GoogleDriveSecret {
    library_id: String,
    client_id: String,
    client_secret: Option<String>,
    access_token: String,
    refresh_token: String,
    access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Deserialize)]
struct GoogleDriveTokenResponse {
    access_token: String,
    expires_in: Option<i64>,
    refresh_token: Option<String>,
}

#[derive(Debug, Deserialize)]
struct GoogleDriveUserInfoResponse {
    user: Option<GoogleDriveUser>,
}

#[derive(Debug, Deserialize)]
struct GoogleDriveUser {
    #[serde(default)]
    email_address: Option<String>,
    #[serde(default)]
    permission_id: Option<String>,
}

#[derive(Debug, Clone, Deserialize)]
struct GoogleDriveFileMetadata {
    id: String,
    name: String,
    #[serde(rename = "mimeType")]
    mime_type: Option<String>,
    #[serde(rename = "headRevisionId")]
    head_revision_id: Option<String>,
    #[serde(default, rename = "modifiedTime")]
    modified_time: Option<String>,
}

#[derive(Debug, Deserialize)]
struct GoogleDriveFileListResponse {
    #[serde(default)]
    files: Vec<GoogleDriveFileMetadata>,
}

const GOOGLE_DRIVE_FOLDER_MIME_TYPE: &str = "application/vnd.google-apps.folder";
const GOOGLE_DRIVE_ROOT_ID: &str = "root";

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum RemoteAuthState {
    Pending,
    Ready,
    Failed,
}

#[derive(Debug, Clone, Serialize)]
pub struct RemoteAuthStart {
    pub session_id: String,
    pub provider: RemoteLibraryProvider,
    pub authorization_url: Option<String>,
    pub expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone, Serialize)]
pub struct RemoteAuthStatus {
    pub session_id: String,
    pub provider: RemoteLibraryProvider,
    pub state: RemoteAuthState,
    pub remote_root_locator: Option<String>,
    pub display_name: Option<String>,
    pub error: Option<CommandError>,
}

#[derive(Debug, Clone, Serialize)]
pub struct RemoteLibraryCandidate {
    pub provider: RemoteLibraryProvider,
    pub remote_root_locator: String,
    pub remote_path_display: String,
    pub display_name: String,
    pub account_id: String,
}

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum UploadState {
    Idle,
    Running,
    Completed,
    Failed,
}

#[derive(Debug, Clone, Serialize)]
pub struct UploadStatusSnapshot {
    pub song_id: String,
    pub state: UploadState,
    pub percent: u8,
    pub remote_library_id: Option<String>,
    pub detail: Option<String>,
    pub error: Option<CommandError>,
}

#[derive(Debug, Clone)]
pub struct RemoteAuthSession {
    pub provider: RemoteLibraryProvider,
    pub state: RemoteAuthState,
    pub remote_root_locator: Option<String>,
    pub display_name: Option<String>,
    pub account_id: String,
    pub error: Option<CommandError>,
    google_drive: Option<GoogleDriveSessionData>,
    webdav: Option<WebDavSessionData>,
}

#[derive(Debug, Clone, Serialize)]
struct UploadProgressPayload {
    song_id: String,
    percent: u8,
    remote_library_id: Option<String>,
    detail: Option<String>,
}

#[derive(Debug, Clone, Serialize)]
struct UploadCompletePayload {
    song_id: String,
    remote_library_id: Option<String>,
}

#[derive(Debug, Clone, Serialize)]
struct UploadErrorPayload {
    song_id: String,
    remote_library_id: Option<String>,
    error: CommandError,
}

fn current_unix_time_ms() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_millis() as i64
}

fn session_id_for_provider(provider: RemoteLibraryProvider) -> String {
    format!(
        "remote-auth-{}-{}",
        provider.as_str(),
        current_unix_time_ms()
    )
}

fn remote_library_id(
    provider: RemoteLibraryProvider,
    account_id: &str,
    remote_root_locator: &str,
) -> String {
    config::library_id_for_path(&format!(
        "remote:{}:{}:{}",
        provider.as_str(),
        account_id,
        remote_root_locator
    ))
}

fn remote_libraries_dir(app_data_dir: &Path) -> PathBuf {
    app_data_dir.join(REMOTE_LIBRARIES_DIR)
}

fn remote_library_root(app_data_dir: &Path, library_id: &str) -> PathBuf {
    remote_libraries_dir(app_data_dir).join(library_id)
}

fn remote_library_secrets_path(app_data_dir: &Path) -> PathBuf {
    app_data_dir.join(REMOTE_LIBRARY_SECRETS_FILENAME)
}

fn load_remote_secrets(app_data_dir: &Path) -> CommandResult<RemoteSecretsFile> {
    let path = remote_library_secrets_path(app_data_dir);
    if !path.exists() {
        return Ok(RemoteSecretsFile::default());
    }

    let contents = fs::read_to_string(&path)
        .map_err(|error| library_error(format!("failed to read {}: {error}", path.display())))?;
    serde_json::from_str(&contents)
        .map_err(|error| library_error(format!("failed to parse {}: {error}", path.display())))
}

fn persist_remote_secrets(app_data_dir: &Path, secrets: &RemoteSecretsFile) -> CommandResult<()> {
    fs::create_dir_all(app_data_dir).map_err(|error| {
        library_error(format!("failed to create {}: {error}", app_data_dir.display()))
    })?;
    let path = remote_library_secrets_path(app_data_dir);
    let json = serde_json::to_string_pretty(secrets)
        .map_err(|error| library_error(format!("failed to serialize remote secrets: {error}")))?;
    fs::write(&path, json)
        .map_err(|error| library_error(format!("failed to write {}: {error}", path.display())))
}

fn normalize_server_url(raw: &str) -> CommandResult<String> {
    let mut url = Url::parse(raw)
        .map_err(|error| library_error(format!("invalid WebDAV server URL: {error}")))?;
    if !raw.ends_with('/') {
        let next_path = format!("{}/", url.path().trim_end_matches('/'));
        url.set_path(&next_path);
    }
    Ok(url.to_string())
}

fn normalize_webdav_root_path(raw: Option<&str>, fallback_display_name: &str) -> String {
    let candidate = raw.unwrap_or_default().trim();
    let value = if candidate.is_empty() {
        slugify_display_name(fallback_display_name)
    } else {
        candidate.trim_matches('/').to_owned()
    };

    if value.is_empty() {
        slugify_display_name(fallback_display_name)
    } else {
        value
    }
}

fn join_url(base: &str, relative: &str) -> CommandResult<String> {
    let base_url = Url::parse(base)
        .map_err(|error| library_error(format!("invalid base URL {base}: {error}")))?;
    base_url
        .join(relative)
        .map(|url| url.to_string())
        .map_err(|error| library_error(format!("failed to join {base} + {relative}: {error}")))
}

fn remote_path_display_from_url(url: &str) -> String {
    match Url::parse(url) {
        Ok(parsed) => {
            let path = parsed.path().trim_end_matches('/');
            if path.is_empty() {
                "/".to_owned()
            } else {
                path.to_owned()
            }
        }
        Err(_) => url.to_owned(),
    }
}

fn webdav_client() -> CommandResult<Client> {
    Client::builder()
        .redirect(reqwest::redirect::Policy::limited(10))
        .build()
        .map_err(|error| library_error(format!("failed to build WebDAV client: {error}")))
}

fn webdav_send(
    client: &Client,
    method: Method,
    url: &str,
    username: &str,
    password: &str,
    headers: Option<HeaderMap>,
    body: Option<Vec<u8>>,
) -> CommandResult<Response> {
    let mut request = client
        .request(method, url)
        .basic_auth(username, Some(password));
    if let Some(headers) = headers {
        request = request.headers(headers);
    }
    if let Some(body) = body {
        request = request.body(body);
    }

    request
        .send()
        .map_err(|error| library_error(format!("request to {url} failed: {error}")))
}

fn webdav_exists(client: &Client, url: &str, username: &str, password: &str) -> CommandResult<bool> {
    let response = webdav_send(client, Method::HEAD, url, username, password, None, None)?;
    Ok(matches!(response.status(), StatusCode::OK | StatusCode::NO_CONTENT | StatusCode::FOUND | StatusCode::MOVED_PERMANENTLY))
}

fn webdav_get_etag(client: &Client, url: &str, username: &str, password: &str) -> CommandResult<Option<String>> {
    let response = webdav_send(client, Method::HEAD, url, username, password, None, None)?;
    if response.status() == StatusCode::NOT_FOUND {
        return Ok(None);
    }
    if !response.status().is_success() {
        return Err(library_error(format!(
            "WebDAV HEAD {} failed with status {}",
            url,
            response.status()
        )));
    }
    Ok(response
        .headers()
        .get(ETAG)
        .and_then(|value| value.to_str().ok())
        .map(str::to_owned))
}

fn ensure_webdav_collection_chain(
    client: &Client,
    server_url: &str,
    root_url: &str,
    username: &str,
    password: &str,
) -> CommandResult<()> {
    let server = Url::parse(server_url)
        .map_err(|error| library_error(format!("invalid WebDAV server URL: {error}")))?;
    let root = Url::parse(root_url)
        .map_err(|error| library_error(format!("invalid WebDAV root URL: {error}")))?;

    let server_segments: Vec<String> = server
        .path_segments()
        .map(|segments| segments.filter(|segment| !segment.is_empty()).map(str::to_owned).collect())
        .unwrap_or_default();
    let root_segments: Vec<String> = root
        .path_segments()
        .map(|segments| segments.filter(|segment| !segment.is_empty()).map(str::to_owned).collect())
        .unwrap_or_default();

    if root_segments.len() < server_segments.len()
        || root_segments[..server_segments.len()] != server_segments[..]
    {
        return Err(library_error(
            "WebDAV library path must stay under the configured server URL".to_owned(),
        ));
    }

    let mut current = server.to_string();
    for segment in &root_segments[server_segments.len()..] {
        current = join_url(&current, &format!("{segment}/"))?;
        let response = webdav_send(client, Method::from_bytes(b"MKCOL").expect("valid method"), &current, username, password, None, None)?;
        match response.status() {
            StatusCode::CREATED | StatusCode::METHOD_NOT_ALLOWED | StatusCode::OK | StatusCode::NO_CONTENT => {}
            status => {
                return Err(library_error(format!(
                    "failed to create WebDAV folder {}: {}",
                    current, status
                )));
            }
        }
    }

    Ok(())
}

fn download_webdav_file(
    client: &Client,
    url: &str,
    destination: &Path,
    username: &str,
    password: &str,
) -> CommandResult<Option<String>> {
    let response = webdav_send(client, Method::GET, url, username, password, None, None)?;
    if response.status() == StatusCode::NOT_FOUND {
        return Ok(None);
    }
    if !response.status().is_success() {
        return Err(library_error(format!(
            "failed to download {}: {}",
            url,
            response.status()
        )));
    }

    if let Some(parent) = destination.parent() {
        fs::create_dir_all(parent).map_err(|error| {
            library_error(format!("failed to create {}: {error}", parent.display()))
        })?;
    }

    let etag = response
        .headers()
        .get(ETAG)
        .and_then(|value| value.to_str().ok())
        .map(str::to_owned);
    let bytes = response
        .bytes()
        .map_err(|error| library_error(format!("failed to read {}: {error}", url)))?;
    let mut file = fs::File::create(destination).map_err(|error| {
        library_error(format!("failed to create {}: {error}", destination.display()))
    })?;
    file.write_all(bytes.as_ref()).map_err(|error| {
        library_error(format!("failed to write {}: {error}", destination.display()))
    })?;
    Ok(etag)
}

fn upload_webdav_file(
    client: &Client,
    url: &str,
    source: &Path,
    username: &str,
    password: &str,
) -> CommandResult<Option<String>> {
    let bytes = fs::read(source)
        .map_err(|error| library_error(format!("failed to read {}: {error}", source.display())))?;
    let response = webdav_send(client, Method::PUT, url, username, password, None, Some(bytes))?;
    match response.status() {
        StatusCode::OK | StatusCode::CREATED | StatusCode::NO_CONTENT | StatusCode::ACCEPTED => Ok(
            response
                .headers()
                .get(ETAG)
                .and_then(|value| value.to_str().ok())
                .map(str::to_owned),
        ),
        status => Err(library_error(format!(
            "failed to upload {} to {}: {}",
            source.display(),
            url,
            status
        ))),
    }
}

fn upload_webdav_bytes(
    client: &Client,
    url: &str,
    bytes: Vec<u8>,
    username: &str,
    password: &str,
) -> CommandResult<()> {
    let response = webdav_send(client, Method::PUT, url, username, password, None, Some(bytes))?;
    match response.status() {
        StatusCode::OK | StatusCode::CREATED | StatusCode::NO_CONTENT | StatusCode::ACCEPTED => Ok(()),
        status => Err(library_error(format!("failed to upload {}: {}", url, status))),
    }
}

fn store_remote_secret(app_data_dir: &Path, secret: WebDavSecret) -> CommandResult<()> {
    let mut secrets = load_remote_secrets(app_data_dir)?;
    secrets.libraries.insert(
        secret.library_id,
        RemoteSecretRecord::WebDav {
            server_url: secret.server_url,
            username: secret.username,
            password: secret.password,
        },
    );
    persist_remote_secrets(app_data_dir, &secrets)
}

fn load_webdav_secret(app_data_dir: &Path, library: &RegisteredLibrary) -> CommandResult<WebDavSecret> {
    let remote_root_url = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator"))?
        .to_owned();
    let secrets = load_remote_secrets(app_data_dir)?;
    let secret = secrets
        .libraries
        .get(library.id())
        .ok_or_else(|| library_error("missing stored credentials for the remote library"))?;

    match secret {
        RemoteSecretRecord::GoogleDrive { .. } => Err(library_error(
            "stored secret is not a WebDAV credential".to_owned(),
        )),
        RemoteSecretRecord::WebDav {
            server_url,
            username,
            password,
        } => Ok(WebDavSecret {
            library_id: library.id().to_owned(),
            server_url: server_url.clone(),
            root_url: remote_root_url,
            username: username.clone(),
            password: password.clone(),
        }),
    }
}

fn load_app_config(app_data_dir: &Path) -> CommandResult<AppConfig> {
    Ok(config::load_config(app_data_dir)
        .map_err(library_error)?
        .unwrap_or_default())
}

fn persist_app_config(app_data_dir: &Path, config: &AppConfig) -> CommandResult<()> {
    config::save_config(app_data_dir, config).map_err(library_error)
}

fn load_remote_root(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<LibraryRoot> {
    let root_path = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let root = if root_path.join(".openkara-library").exists() {
        LibraryRoot::open(&root_path).map_err(library_error)?
    } else {
        LibraryRoot::create(&root_path).map_err(library_error)?
    };
    cache::initialize_library_database(&root.database_path()).map_err(library_error)?;

    // Ensure the directory structure exists even if the cached copy was created
    // before the remote library folder layout stabilized.
    let _ = app_data_dir;
    Ok(root)
}

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
            _ => remote_root_locator,
        },
        display_name: display_name.to_owned(),
        account_id: session.account_id.clone(),
    }
}

fn random_token(length: usize) -> String {
    rand::rng()
        .sample_iter(&Alphanumeric)
        .take(length)
        .map(char::from)
        .collect()
}

fn google_drive_code_challenge(code_verifier: &str) -> String {
    let digest = sha2::Sha256::digest(code_verifier.as_bytes());
    URL_SAFE_NO_PAD.encode(digest)
}

fn build_google_drive_authorization_url(session: &GoogleDriveSessionData) -> CommandResult<String> {
    let mut url = Url::parse("https://accounts.google.com/o/oauth2/v2/auth")
        .map_err(|error| library_error(format!("failed to build Google auth URL: {error}")))?;
    url.query_pairs_mut()
        .append_pair("client_id", &session.client_id)
        .append_pair("redirect_uri", &session.redirect_uri)
        .append_pair("response_type", "code")
        .append_pair("scope", "https://www.googleapis.com/auth/drive")
        .append_pair("access_type", "offline")
        .append_pair("prompt", "consent")
        .append_pair("code_challenge", &google_drive_code_challenge(&session.code_verifier))
        .append_pair("code_challenge_method", "S256")
        .append_pair("state", &session.state_token);
    Ok(url.to_string())
}

fn parse_google_drive_payload(
    payload: Option<serde_json::Value>,
) -> CommandResult<GoogleDriveSessionData> {
    let payload = payload.ok_or_else(|| {
        library_error("Google Drive client configuration is required for this provider".to_owned())
    })?;

    match serde_json::from_value::<RemoteAuthPayloadInput>(payload)
        .map_err(|error| library_error(format!("invalid remote auth payload: {error}")))?
    {
        RemoteAuthPayloadInput::GoogleDrive {
            client_id,
            client_secret,
        } => {
            if client_id.trim().is_empty() {
                return Err(library_error("Google Drive client ID cannot be empty".to_owned()));
            }

            Ok(GoogleDriveSessionData {
                client_id,
                client_secret,
                code_verifier: random_token(64),
                redirect_uri: String::new(),
                state_token: random_token(48),
                root_folder_id: None,
                access_token: None,
                refresh_token: None,
                access_token_expires_at_ms: None,
            })
        }
        RemoteAuthPayloadInput::WebDav { .. } => Err(library_error(
            "expected a Google Drive auth payload".to_owned(),
        )),
    }
}

fn parse_webdav_payload(payload: Option<serde_json::Value>) -> CommandResult<WebDavSessionData> {
    let payload = payload.ok_or_else(|| {
        library_error("WebDAV connection details are required for this provider".to_owned())
    })?;

    match serde_json::from_value::<RemoteAuthPayloadInput>(payload)
        .map_err(|error| library_error(format!("invalid remote auth payload: {error}")))?
    {
        RemoteAuthPayloadInput::WebDav {
            server_url,
            username,
            password,
            root_path,
        } => {
            if server_url.trim().is_empty() {
                return Err(library_error("WebDAV server URL cannot be empty".to_owned()));
            }
            if username.trim().is_empty() {
                return Err(library_error("WebDAV username cannot be empty".to_owned()));
            }
            if password.trim().is_empty() {
                return Err(library_error("WebDAV password cannot be empty".to_owned()));
            }

            Ok(WebDavSessionData {
                server_url: normalize_server_url(&server_url)?,
                username,
                password,
                root_path: root_path.map(|value| value.trim().to_owned()),
            })
        }
        RemoteAuthPayloadInput::GoogleDrive { .. } => Err(library_error(
            "expected a WebDAV auth payload".to_owned(),
        )),
    }
}

fn google_drive_store_secret(app_data_dir: &Path, secret: GoogleDriveSecret) -> CommandResult<()> {
    let mut secrets = load_remote_secrets(app_data_dir)?;
    secrets.libraries.insert(
        secret.library_id,
        RemoteSecretRecord::GoogleDrive {
            client_id: secret.client_id,
            client_secret: secret.client_secret,
            access_token: secret.access_token,
            refresh_token: secret.refresh_token,
            access_token_expires_at_ms: secret.access_token_expires_at_ms,
        },
    );
    persist_remote_secrets(app_data_dir, &secrets)
}

fn load_google_drive_secret(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<GoogleDriveSecret> {
    let secrets = load_remote_secrets(app_data_dir)?;
    let secret = secrets
        .libraries
        .get(library.id())
        .ok_or_else(|| library_error("missing stored credentials for the remote library"))?;

    match secret {
        RemoteSecretRecord::GoogleDrive {
            client_id,
            client_secret,
            access_token,
            refresh_token,
            access_token_expires_at_ms,
        } => Ok(GoogleDriveSecret {
            library_id: library.id().to_owned(),
            client_id: client_id.clone(),
            client_secret: client_secret.clone(),
            access_token: access_token.clone(),
            refresh_token: refresh_token.clone(),
            access_token_expires_at_ms: *access_token_expires_at_ms,
        }),
        RemoteSecretRecord::WebDav { .. } => Err(library_error(
            "stored secret is not a Google Drive credential".to_owned(),
        )),
    }
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

    let mut encoded = Url::parse("https://example.invalid")
        .map_err(|error| library_error(format!("failed to build token body: {error}")))?;
    {
        let mut pairs = encoded.query_pairs_mut();
        for (key, value) in &params {
            pairs.append_pair(key, value);
        }
    }
    let body = encoded.query().unwrap_or_default().to_owned();

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
    google_drive_store_secret(app_data_dir, secret.clone())?;
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
            "mimeType": "application/vnd.google-apps.folder",
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
        Some("application/vnd.google-apps.folder"),
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

fn google_drive_get_or_create_folder_with_token(
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
    let url = google_drive_api_url("/drive/v3/files?fields=id,name,mimeType,headRevisionId,modifiedTime")?;
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

fn google_drive_download_file(
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

fn google_drive_root_display_name(display_name: &str) -> String {
    format!("My Drive/{display_name}")
}

fn update_remote_auth_session(
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

    let mut encoded = Url::parse("https://example.invalid")
        .map_err(|error| library_error(format!("failed to build token body: {error}")))?;
    {
        let mut pairs = encoded.query_pairs_mut();
        for (key, value) in &params {
            pairs.append_pair(key, value);
        }
    }
    let body = encoded.query().unwrap_or_default().to_owned();

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
    let mut url = google_drive_api_url("/drive/v3/about")?;
    url.query_pairs_mut().append_pair("fields", "user");
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
    let user = body
        .user
        .ok_or_else(|| library_error("Google Drive account response did not include user info".to_owned()))?;
    user.email_address
        .or(user.permission_id)
        .ok_or_else(|| library_error("Google Drive account info did not include a stable identifier".to_owned()))
}

fn google_drive_callback_response(body: &str) -> TinyHttpResponse<std::io::Cursor<Vec<u8>>> {
    TinyHttpResponse::from_string(body.to_owned())
}

fn spawn_google_drive_auth_worker(
    sessions: Arc<Mutex<HashMap<String, RemoteAuthSession>>>,
    session_id: String,
    session: GoogleDriveSessionData,
) -> CommandResult<GoogleDriveSessionData> {
    let listener = TcpListener::bind(SocketAddrV4::new(Ipv4Addr::LOCALHOST, 0)).map_err(|error| {
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
        let request = match server.recv_timeout(std::time::Duration::from_secs(300)) {
            Ok(Some(request)) => request,
            Ok(None) => {
                update_remote_auth_session(&sessions, &session_id, |state| {
                    state.state = RemoteAuthState::Failed;
                    state.error = Some(library_error(
                        "Google sign-in timed out before the browser returned to OpenKara."
                            .to_owned(),
                    ));
                });
                return;
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
        };

        let callback_url = format!("http://127.0.0.1:{}{}", port, request.url());
        let parsed = match Url::parse(&callback_url) {
            Ok(parsed) => parsed,
            Err(error) => {
                let _ = request.respond(google_drive_callback_response("Invalid OAuth callback."));
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
            let _ = request.respond(google_drive_callback_response("OAuth state mismatch."));
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
            let _ = request.respond(google_drive_callback_response("Google sign-in was cancelled or denied."));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(format!("Google sign-in failed: {error}")));
            });
            return;
        }

        let Some(code) = query.get("code") else {
            let _ = request.respond(google_drive_callback_response("Missing Google authorization code."));
            update_remote_auth_session(&sessions, &session_id, |state| {
                state.state = RemoteAuthState::Failed;
                state.error = Some(library_error(
                    "Google sign-in did not return an authorization code."
                        .to_owned(),
                ));
            });
            return;
        };

        match google_drive_exchange_code_for_tokens(&worker_session, code)
            .and_then(|tokens| {
                let account_id = google_drive_fetch_account_id(&tokens.access_token)?;
                Ok((tokens, account_id))
            }) {
            Ok((tokens, account_id)) => {
                let _ = request.respond(google_drive_callback_response(
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
                let _ = request.respond(google_drive_callback_response(
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

fn slugify_display_name(display_name: &str) -> String {
    let mut slug = String::new();
    let mut last_was_dash = false;

    for ch in display_name.trim().chars() {
        if ch.is_ascii_alphanumeric() {
            slug.push(ch.to_ascii_lowercase());
            last_was_dash = false;
        } else if !last_was_dash && !slug.is_empty() {
            slug.push('-');
            last_was_dash = true;
        }
    }

    if slug.ends_with('-') {
        slug.pop();
    }

    if slug.is_empty() {
        "remote-library".to_owned()
    } else {
        slug
    }
}

fn mark_upload_status(
    state: &State<'_, AppState>,
    song_id: &str,
    remote_library_id: Option<String>,
    upload_state: UploadState,
    percent: u8,
    detail: Option<String>,
    error: Option<CommandError>,
) -> CommandResult<UploadStatusSnapshot> {
    let snapshot = UploadStatusSnapshot {
        song_id: song_id.to_owned(),
        state: upload_state,
        percent,
        remote_library_id,
        detail,
        error,
    };

    let mut guard = state
        .remote_upload_statuses
        .lock()
        .map_err(|_| state_lock_error("remote upload status lock was poisoned"))?;
    guard.insert(song_id.to_owned(), snapshot.clone());
    Ok(snapshot)
}

fn emit_upload_progress<R: tauri::Runtime>(
    app_handle: &AppHandle<R>,
    snapshot: &UploadStatusSnapshot,
) {
    let payload = UploadProgressPayload {
        song_id: snapshot.song_id.clone(),
        percent: snapshot.percent,
        remote_library_id: snapshot.remote_library_id.clone(),
        detail: snapshot.detail.clone(),
    };
    let _ = app_handle.emit("upload-progress", payload);
}

fn emit_upload_complete<R: tauri::Runtime>(
    app_handle: &AppHandle<R>,
    snapshot: &UploadStatusSnapshot,
) {
    let payload = UploadCompletePayload {
        song_id: snapshot.song_id.clone(),
        remote_library_id: snapshot.remote_library_id.clone(),
    };
    let _ = app_handle.emit("upload-complete", payload);
}

fn emit_upload_error<R: tauri::Runtime>(
    app_handle: &AppHandle<R>,
    snapshot: &UploadStatusSnapshot,
    error: CommandError,
) {
    let payload = UploadErrorPayload {
        song_id: snapshot.song_id.clone(),
        remote_library_id: snapshot.remote_library_id.clone(),
        error,
    };
    let _ = app_handle.emit("upload-error", payload);
}

fn copy_file_if_present(source: Option<&Path>, destination: &Path) -> CommandResult<()> {
    let Some(source) = source else {
        return Ok(());
    };

    if let Some(parent) = destination.parent() {
        fs::create_dir_all(parent).map_err(|error| {
            library_error(format!("failed to create {}: {error}", parent.display()))
        })?;
    }

    fs::copy(source, destination).map_err(|error| {
        library_error(format!(
            "failed to copy {} to {}: {error}",
            source.display(),
            destination.display()
        ))
    })?;
    Ok(())
}

fn copy_directory_recursive(source: &Path, destination: &Path) -> CommandResult<()> {
    if !source.exists() {
        return Err(library_error(format!(
            "source directory {} does not exist",
            source.display()
        )));
    }

    if destination.exists() {
        fs::remove_dir_all(destination).map_err(|error| {
            library_error(format!(
                "failed to clear destination directory {}: {error}",
                destination.display()
            ))
        })?;
    }
    fs::create_dir_all(destination).map_err(|error| {
        library_error(format!(
            "failed to create destination directory {}: {error}",
            destination.display()
        ))
    })?;

    for entry in fs::read_dir(source).map_err(|error| {
        library_error(format!(
            "failed to read directory {}: {error}",
            source.display()
        ))
    })? {
        let entry = entry.map_err(|error| library_error(error.to_string()))?;
        let source_path = entry.path();
        let destination_path = destination.join(entry.file_name());

        if source_path.is_dir() {
            copy_directory_recursive(&source_path, &destination_path)?;
        } else {
            if let Some(parent) = destination_path.parent() {
                fs::create_dir_all(parent).map_err(|error| {
                    library_error(format!(
                        "failed to create destination directory {}: {error}",
                        parent.display()
                    ))
                })?;
            }
            fs::copy(&source_path, &destination_path).map_err(|error| {
                library_error(format!(
                    "failed to copy {} to {}: {error}",
                    source_path.display(),
                    destination_path.display()
                ))
            })?;
        }
    }

    Ok(())
}

fn upsert_stem_entry(
    connection: &rusqlite::Connection,
    entry: &cache::stems::StemCacheEntry,
) -> CommandResult<()> {
    connection
        .execute(
            "INSERT INTO stems (
                song_hash,
                vocals_path,
                accomp_path,
                separated_at,
                drums_path,
                bass_path,
                other_path,
                model_variant
            ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)
            ON CONFLICT(song_hash) DO UPDATE SET
                vocals_path = excluded.vocals_path,
                accomp_path = excluded.accomp_path,
                separated_at = excluded.separated_at,
                drums_path = excluded.drums_path,
                bass_path = excluded.bass_path,
                other_path = excluded.other_path,
                model_variant = excluded.model_variant",
            rusqlite::params![
                entry.song_hash,
                entry.vocals_path,
                entry.accomp_path,
                entry.separated_at,
                entry.drums_path,
                entry.bass_path,
                entry.other_path,
                entry.model_variant,
            ],
        )
        .map_err(|error| database_error(error.to_string()))?;
    Ok(())
}

fn webdav_marker_url(root_url: &str) -> CommandResult<String> {
    join_url(root_url, ".openkara-library")
}

fn webdav_database_url(root_url: &str) -> CommandResult<String> {
    join_url(root_url, "openkara.db")
}

fn initialize_or_sync_webdav_library(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &WebDavSecret,
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

    let client = webdav_client()?;
    ensure_webdav_collection_chain(
        &client,
        &secret.server_url,
        &secret.root_url,
        &secret.username,
        &secret.password,
    )?;

    for directory in ["media", "media-g", "stems"] {
        let directory_url = join_url(&secret.root_url, &format!("{directory}/"))?;
        ensure_webdav_collection_chain(
            &client,
            &secret.server_url,
            &directory_url,
            &secret.username,
            &secret.password,
        )?;
    }

    let marker_url = webdav_marker_url(&secret.root_url)?;
    if !webdav_exists(&client, &marker_url, &secret.username, &secret.password)? {
        upload_webdav_bytes(
            &client,
            &marker_url,
            b"openkara remote library\n".to_vec(),
            &secret.username,
            &secret.password,
        )?;
    }

    let database_url = webdav_database_url(&secret.root_url)?;
    let etag = if webdav_exists(&client, &database_url, &secret.username, &secret.password)? {
        download_webdav_file(
            &client,
            &database_url,
            &root.database_path(),
            &secret.username,
            &secret.password,
        )?
    } else {
        upload_webdav_file(
            &client,
            &database_url,
            &root.database_path(),
            &secret.username,
            &secret.password,
        )?
    };

    let _ = app_data_dir;
    Ok(etag)
}

fn google_drive_get_file_by_id(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    file_id: &str,
) -> CommandResult<GoogleDriveFileMetadata> {
    let mut url = google_drive_api_url(&format!("/drive/v3/files/{file_id}"))?;
    url.query_pairs_mut()
        .append_pair("fields", "id,name,mimeType,headRevisionId,modifiedTime");
    let response = google_drive_authorized_request(app_data_dir, secret, Method::GET, url)?
        .send()
        .map_err(|error| library_error(format!("failed to fetch Google Drive metadata: {error}")))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "Google Drive metadata lookup failed with status {}",
            response.status()
        )));
    }
    response
        .json()
        .map_err(|error| library_error(format!("failed to parse file metadata: {error}")))
}

fn google_drive_find_relative_entry(
    app_data_dir: &Path,
    secret: &mut GoogleDriveSecret,
    root_folder_id: &str,
    relative_path: &str,
) -> CommandResult<Option<GoogleDriveFileMetadata>> {
    let segments: Vec<&str> = relative_path.split('/').filter(|segment| !segment.is_empty()).collect();
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
            if is_last { None } else { Some(GOOGLE_DRIVE_FOLDER_MIME_TYPE) },
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

fn google_drive_upload_relative_file_to_remote(
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

    let segments: Vec<&str> = relative_path.split('/').filter(|segment| !segment.is_empty()).collect();
    if segments.is_empty() {
        return Ok(());
    }
    let file_name = segments.last().copied().unwrap_or_default();
    let mut parent_id = root_folder_id.to_owned();
    for segment in &segments[..segments.len() - 1] {
        let folder = google_drive_get_or_create_folder(app_data_dir, &mut secret, &parent_id, segment)?;
        parent_id = folder.id;
    }

    let file = match google_drive_find_child(app_data_dir, &mut secret, &parent_id, file_name, None)? {
        Some(file) => file,
        None => google_drive_create_empty_file(app_data_dir, &mut secret, &parent_id, file_name)?,
    };
    let _ = google_drive_upload_file_bytes(app_data_dir, &mut secret, &file.id, bytes)?;
    Ok(())
}

fn google_drive_upload_directory_to_remote(
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
            google_drive_upload_directory_to_remote(app_data_dir, library, secret, &relative, root_folder_id)?;
        } else {
            google_drive_upload_relative_file_to_remote(app_data_dir, library, secret, &relative, root_folder_id)?;
        }
    }
    Ok(())
}

fn initialize_or_sync_google_drive_library(
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

    if google_drive_find_relative_entry(app_data_dir, &mut secret, root_folder_id, ".openkara-library")?.is_none() {
        let marker_path = root.resolve(".openkara-library");
        fs::write(&marker_path, b"openkara remote library\n").map_err(|error| {
            library_error(format!("failed to write {}: {error}", marker_path.display()))
        })?;
        google_drive_upload_relative_file_to_remote(app_data_dir, library, &secret, ".openkara-library", root_folder_id)?;
    }

    let database_entry = google_drive_find_relative_entry(app_data_dir, &mut secret, root_folder_id, "openkara.db")?;
    let revision = if let Some(database_entry) = database_entry {
        google_drive_download_file(app_data_dir, &mut secret, &database_entry.id, &root.database_path())?;
        database_entry.head_revision_id.or(database_entry.modified_time)
    } else {
        google_drive_upload_relative_file_to_remote(app_data_dir, library, &secret, "openkara.db", root_folder_id)?;
        let uploaded = google_drive_find_relative_entry(app_data_dir, &mut secret, root_folder_id, "openkara.db")?
            .ok_or_else(|| library_error("Google Drive database upload succeeded but the file was not found afterwards".to_owned()))?;
        uploaded.head_revision_id.or(uploaded.modified_time)
    };

    Ok(revision)
}

fn update_remote_revision_in_config(
    app_data_dir: &Path,
    library_id: &str,
    remote_revision: Option<String>,
) -> CommandResult<()> {
    let mut config = load_app_config(app_data_dir)?;
    if let Some(library) = config
        .libraries
        .iter_mut()
        .find(|entry| entry.id() == library_id)
    {
        if let RegisteredLibrary::Remote {
            remote_revision: revision,
            ..
        } = library
        {
            *revision = remote_revision.or_else(|| Some(current_unix_time_ms().to_string()));
        }
    }
    persist_app_config(app_data_dir, &config)
}

fn upload_relative_file_to_remote(
    library: &RegisteredLibrary,
    secret: &WebDavSecret,
    relative_path: &str,
) -> CommandResult<()> {
    let local_root = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let source = local_root.join(relative_path);
    let client = webdav_client()?;
    if let Some(parent) = Path::new(relative_path).parent() {
        let mut current = String::new();
        for segment in parent.iter().filter_map(|segment| segment.to_str()) {
            if !current.is_empty() {
                current.push('/');
            }
            current.push_str(segment);
            let directory_url = join_url(&secret.root_url, &format!("{current}/"))?;
            ensure_webdav_collection_chain(
                &client,
                &secret.server_url,
                &directory_url,
                &secret.username,
                &secret.password,
            )?;
        }
    }
    let file_url = join_url(&secret.root_url, relative_path)?;
    upload_webdav_file(&client, &file_url, &source, &secret.username, &secret.password)?;
    Ok(())
}

fn upload_directory_to_remote(
    library: &RegisteredLibrary,
    secret: &WebDavSecret,
    relative_directory: &str,
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
            upload_directory_to_remote(library, secret, &relative)?;
        } else {
            upload_relative_file_to_remote(library, secret, &relative)?;
        }
    }

    Ok(())
}

fn upload_remote_database(app_data_dir: &Path, library: &RegisteredLibrary) -> CommandResult<()> {
    match library.provider() {
        Some(RemoteLibraryProvider::WebDav) => {
            let secret = load_webdav_secret(app_data_dir, library)?;
            upload_relative_file_to_remote(library, &secret, "openkara.db")?;
            update_remote_revision_in_config(
                app_data_dir,
                library.id(),
                webdav_get_etag(
                    &webdav_client()?,
                    &webdav_database_url(&secret.root_url)?,
                    &secret.username,
                    &secret.password,
                )?,
            )
        }
        Some(RemoteLibraryProvider::GoogleDrive) => {
            let secret = load_google_drive_secret(app_data_dir, library)?;
            let root_folder_id = library
                .remote_root_locator()
                .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
            google_drive_upload_relative_file_to_remote(
                app_data_dir,
                library,
                &secret,
                "openkara.db",
                root_folder_id,
            )?;
            let mut refreshed_secret = load_google_drive_secret(app_data_dir, library)?;
            let metadata = google_drive_find_relative_entry(
                app_data_dir,
                &mut refreshed_secret,
                root_folder_id,
                "openkara.db",
            )?
            .ok_or_else(|| library_error("Google Drive database file is missing after upload".to_owned()))?;
            update_remote_revision_in_config(
                app_data_dir,
                library.id(),
                metadata.head_revision_id.or(metadata.modified_time),
            )
        }
        _ => Err(library_error(
            "the active remote provider is not supported for database upload".to_owned(),
        )),
    }
}

fn active_remote_library(app_data_dir: &Path) -> CommandResult<Option<RegisteredLibrary>> {
    let config = load_app_config(app_data_dir)?;
    let Some(active_library) = config.active_library() else {
        return Ok(None);
    };
    if !matches!(active_library, RegisteredLibrary::Remote { .. }) {
        return Ok(None);
    }
    Ok(Some(active_library.clone()))
}

pub fn sync_active_remote_database_if_needed(app_data_dir: &Path) -> CommandResult<()> {
    let Some(library) = active_remote_library(app_data_dir)? else {
        return Ok(());
    };
    upload_remote_database(app_data_dir, &library)
}

pub fn ensure_remote_file_cached(app_data_dir: &Path, relative_path: &str) -> CommandResult<()> {
    let Some(library) = active_remote_library(app_data_dir)? else {
        return Ok(());
    };
    let root = load_remote_root(app_data_dir, &library)?;
    let destination = root.resolve(relative_path);
    if destination.exists() {
        return Ok(());
    }

    match library.provider() {
        Some(RemoteLibraryProvider::WebDav) => {
            let secret = load_webdav_secret(app_data_dir, &library)?;
            let client = webdav_client()?;
            let file_url = join_url(&secret.root_url, relative_path)?;
            download_webdav_file(
                &client,
                &file_url,
                &destination,
                &secret.username,
                &secret.password,
            )?
            .ok_or_else(|| library_error(format!("remote file {relative_path} was not found")))?;
            Ok(())
        }
        Some(RemoteLibraryProvider::GoogleDrive) => {
            let mut secret = load_google_drive_secret(app_data_dir, &library)?;
            let root_folder_id = library
                .remote_root_locator()
                .ok_or_else(|| library_error("remote library is missing a remote locator".to_owned()))?;
            let entry = google_drive_find_relative_entry(
                app_data_dir,
                &mut secret,
                root_folder_id,
                relative_path,
            )?
            .ok_or_else(|| library_error(format!("remote file {relative_path} was not found")))?;
            google_drive_download_file(app_data_dir, &mut secret, &entry.id, &destination)
        }
        _ => Err(library_error(
            "the active remote provider is not supported for file caching".to_owned(),
        )),
    }
}

fn resolve_remote_binding(
    config: &AppConfig,
    active_library_id: Option<&str>,
) -> Option<RegisteredLibrary> {
    if let Some(active_library_id) = active_library_id {
        if let Some(remote_library) = config.libraries.iter().find(|library| {
            matches!(library, RegisteredLibrary::Remote { bound_local_library_id: Some(bound_local_library_id), .. } if bound_local_library_id == active_library_id)
        }) {
            return Some(remote_library.clone());
        }
    }

    config.active_library().and_then(|library| match library {
        RegisteredLibrary::Remote { .. } => Some(library.clone()),
        RegisteredLibrary::Local { .. } => None,
    })
}

fn copy_remote_song_assets(
    local_root: &LibraryRoot,
    remote_root: &LibraryRoot,
    source_relative_path: &str,
    destination_relative_path: &str,
) -> CommandResult<()> {
    let source_path = local_root.resolve(source_relative_path);
    let destination_path = remote_root.resolve(destination_relative_path);
    copy_file_if_present(Some(&source_path), &destination_path)
}

fn update_remote_song(
    connection: &rusqlite::Connection,
    mut song: Song,
    remote_mode: &str,
) -> CommandResult<()> {
    song.audio_source_kind = remote_mode.to_owned();
    if remote_mode == "stems_remote" {
        song.file_path = None;
    }
    cache::upsert_song(connection, &song).map_err(|error| database_error(error.to_string()))?;
    Ok(())
}

fn publish_song_internal<R: tauri::Runtime>(
    state: &State<'_, AppState>,
    app_handle: &AppHandle<R>,
    song_id: &str,
) -> CommandResult<UploadStatusSnapshot> {
    let config = load_app_config(&state.app_data_dir)?;
    let active_library_id = config
        .active_library()
        .map(|library| library.id().to_owned());
    let remote_library = resolve_remote_binding(&config, active_library_id.as_deref())
        .ok_or_else(|| library_error("no bound remote library is available for publishing"))?;

    let local_root = state.library_root()?;
    let remote_library_id = remote_library.id().to_owned();
    let remote_root = load_remote_root(&state.app_data_dir, &remote_library)?;
    let local_connection = cache::open_database(&local_root.database_path())
        .map_err(|error| database_error(error.to_string()))?;
    let remote_connection = cache::open_database(&remote_root.database_path())
        .map_err(|error| database_error(error.to_string()))?;

    let song = cache::get_song_by_hash(&local_connection, song_id)
        .map_err(|error| database_error(error.to_string()))?
        .ok_or_else(|| library_error(format!("song {song_id} was not found")))?;

    let running = mark_upload_status(
        state,
        song_id,
        Some(remote_library_id.clone()),
        UploadState::Running,
        0,
        Some("Preparing remote publish".to_owned()),
        None,
    )?;
    emit_upload_progress(app_handle, &running);

    let publish_result = if song.is_separable() {
        let stem_entry = cache::stems::get_cached_stem_entry(&local_connection, song_id)
            .map_err(|error| database_error(error.to_string()))?
            .ok_or_else(|| {
                library_error(format!(
                    "song {song_id} must have cached stems before publishing to a remote library"
                ))
            })?;
        let source_stems_dir = local_root.resolve(&format!("stems/{song_id}"));
        let destination_stems_dir = remote_root.resolve(&format!("stems/{song_id}"));
        copy_directory_recursive(&source_stems_dir, &destination_stems_dir)?;
        upsert_stem_entry(&remote_connection, &stem_entry)?;

        update_remote_song(&remote_connection, song.clone(), "stems_remote")?;
        match remote_library.provider() {
            Some(RemoteLibraryProvider::WebDav) => {
                let remote_secret = load_webdav_secret(&state.app_data_dir, &remote_library)?;
                upload_directory_to_remote(&remote_library, &remote_secret, &format!("stems/{song_id}"))?;
            }
            Some(RemoteLibraryProvider::GoogleDrive) => {
                let remote_secret = load_google_drive_secret(&state.app_data_dir, &remote_library)?;
                let root_folder_id = remote_library.remote_root_locator().ok_or_else(|| {
                    library_error("remote library is missing a remote locator".to_owned())
                })?;
                google_drive_upload_directory_to_remote(
                    &state.app_data_dir,
                    &remote_library,
                    &remote_secret,
                    &format!("stems/{song_id}"),
                    root_folder_id,
                )?;
            }
            _ => {
                return Err(library_error(
                    "the bound remote provider is not supported for publishing".to_owned(),
                ));
            }
        }
        Ok::<_, CommandError>(())
    } else {
        if let Some(file_path) = song.file_path.as_deref() {
            copy_remote_song_assets(&local_root, &remote_root, file_path, file_path)?;
            match remote_library.provider() {
                Some(RemoteLibraryProvider::WebDav) => {
                    let remote_secret = load_webdav_secret(&state.app_data_dir, &remote_library)?;
                    upload_relative_file_to_remote(&remote_library, &remote_secret, file_path)?;
                }
                Some(RemoteLibraryProvider::GoogleDrive) => {
                    let remote_secret = load_google_drive_secret(&state.app_data_dir, &remote_library)?;
                    let root_folder_id = remote_library.remote_root_locator().ok_or_else(|| {
                        library_error("remote library is missing a remote locator".to_owned())
                    })?;
                    google_drive_upload_relative_file_to_remote(
                        &state.app_data_dir,
                        &remote_library,
                        &remote_secret,
                        file_path,
                        root_folder_id,
                    )?;
                }
                _ => {
                    return Err(library_error(
                        "the bound remote provider is not supported for publishing".to_owned(),
                    ));
                }
            }
        }
        if let Some(cdg_path) = song.cdg_path.as_deref() {
            copy_remote_song_assets(&local_root, &remote_root, cdg_path, cdg_path)?;
            match remote_library.provider() {
                Some(RemoteLibraryProvider::WebDav) => {
                    let remote_secret = load_webdav_secret(&state.app_data_dir, &remote_library)?;
                    upload_relative_file_to_remote(&remote_library, &remote_secret, cdg_path)?;
                }
                Some(RemoteLibraryProvider::GoogleDrive) => {
                    let remote_secret = load_google_drive_secret(&state.app_data_dir, &remote_library)?;
                    let root_folder_id = remote_library.remote_root_locator().ok_or_else(|| {
                        library_error("remote library is missing a remote locator".to_owned())
                    })?;
                    google_drive_upload_relative_file_to_remote(
                        &state.app_data_dir,
                        &remote_library,
                        &remote_secret,
                        cdg_path,
                        root_folder_id,
                    )?;
                }
                _ => {
                    return Err(library_error(
                        "the bound remote provider is not supported for publishing".to_owned(),
                    ));
                }
            }
        }

        update_remote_song(&remote_connection, song.clone(), "original_remote")?;
        Ok(())
    };

    if let Err(error) = publish_result {
        let failure = mark_upload_status(
            state,
            song_id,
            Some(remote_library_id.clone()),
            UploadState::Failed,
            0,
            None,
            Some(error.clone()),
        )?;
        emit_upload_error(app_handle, &failure, error.clone());
        return Err(error);
    }

    let completed = mark_upload_status(
        state,
        song_id,
        Some(remote_library_id.clone()),
        UploadState::Completed,
        100,
        None,
        None,
    )?;
    emit_upload_complete(app_handle, &completed);

    upload_remote_database(&state.app_data_dir, &remote_library)?;

    Ok(completed)
}

#[tauri::command]
pub fn begin_remote_auth(
    state: State<'_, AppState>,
    provider: RemoteLibraryProvider,
    payload: Option<serde_json::Value>,
) -> CommandResult<RemoteAuthStart> {
    let session_id = session_id_for_provider(provider);
    let mut google_drive = None;
    let webdav = match provider {
        RemoteLibraryProvider::GoogleDrive => {
            let google = parse_google_drive_payload(payload)?;
            google_drive = Some(spawn_google_drive_auth_worker(
                Arc::clone(&state.remote_auth_sessions),
                session_id.clone(),
                google,
            )?);
            None
        }
        RemoteLibraryProvider::WebDav => {
            let payload = parse_webdav_payload(payload)?;
            let client = webdav_client()?;
            let response = webdav_send(
                &client,
                Method::HEAD,
                &payload.server_url,
                &payload.username,
                &payload.password,
                None,
                None,
            )?;
            match response.status() {
                StatusCode::OK
                | StatusCode::NO_CONTENT
                | StatusCode::METHOD_NOT_ALLOWED
                | StatusCode::FOUND
                | StatusCode::MOVED_PERMANENTLY => Some(payload),
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
        RemoteLibraryProvider::Dropbox => {
            return Err(library_error(
                "Dropbox support is still pending. This build only wires real WebDAV remote libraries."
                    .to_owned(),
            ))
        }
    };
    if provider == RemoteLibraryProvider::GoogleDrive {
        let google = google_drive
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
            authorization_url: Some(build_google_drive_authorization_url(&google)?),
            expires_at_ms: Some(current_unix_time_ms() + 15 * 60 * 1000),
        });
    }

    let session = RemoteAuthSession {
        provider,
        state: RemoteAuthState::Pending,
        remote_root_locator: None,
        display_name: None,
        account_id: if let Some(webdav) = &webdav {
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
        webdav,
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

#[tauri::command]
pub fn poll_remote_auth(
    state: State<'_, AppState>,
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

#[tauri::command]
pub fn list_remote_library_roots(
    state: State<'_, AppState>,
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
                RemoteLibraryProvider::GoogleDrive => google_drive_root_display_name(&display_name),
                _ => remote_root_locator,
            },
            display_name,
            account_id: session.account_id.clone(),
        }]);
    }

    Ok(Vec::new())
}

#[tauri::command]
pub fn create_remote_library(
    state: State<'_, AppState>,
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
        RemoteLibraryProvider::WebDav => {
            let webdav = session
                .webdav
                .as_ref()
                .ok_or_else(|| library_error("missing WebDAV session details".to_owned()))?;
            let root_path = normalize_webdav_root_path(webdav.root_path.as_deref(), &display_name);
            join_url(&webdav.server_url, &format!("{root_path}/"))?
        }
        _ => format!(
            "{}:{}:{}",
            session.provider.as_str(),
            session.account_id,
            slugify_display_name(&display_name)
        ),
    };
    session.state = RemoteAuthState::Ready;
    session.remote_root_locator = Some(remote_root_locator.clone());
    session.display_name = Some(display_name.clone());

    Ok(candidate_from_session(&session_id, session, &display_name))
}

#[tauri::command]
pub fn register_remote_library(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    session_id: String,
    remote_root_locator: String,
    display_name: Option<String>,
) -> CommandResult<crate::commands::library_setup::LibraryRegistrySnapshot> {
    let mut sessions = state
        .remote_auth_sessions
        .lock()
        .map_err(|_| state_lock_error("remote auth session lock was poisoned"))?;
    let (default_display_name, account_id, provider, webdav, google_drive) = {
        let session = sessions.get(&session_id).ok_or_else(|| {
            library_error(format!("remote auth session {session_id} was not found"))
        })?;
        (
            session.display_name.clone(),
            session.account_id.clone(),
            session.provider,
            session.webdav.clone(),
            session.google_drive.clone(),
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

    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| library_error(error.to_string()))?;
    fs::create_dir_all(remote_libraries_dir(&app_data_dir)).map_err(|error| {
        library_error(format!(
            "failed to create remote library root at {}: {error}",
            remote_libraries_dir(&app_data_dir).display()
        ))
    })?;

    let library_id = remote_library_id(provider, &account_id, &remote_root_locator);
    let root_path = remote_library_root(&app_data_dir, &library_id);
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
    let provisional_library = RegisteredLibrary::remote(
        library_id.clone(),
        display_name.clone(),
        provider,
        account_id.clone(),
        remote_root_locator.clone(),
        remote_path_display.clone(),
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
            google_drive_store_secret(
                &app_data_dir,
                GoogleDriveSecret {
                    library_id: library_id.clone(),
                    client_id: google.client_id,
                    client_secret: google.client_secret,
                    access_token,
                    refresh_token,
                    access_token_expires_at_ms: google.access_token_expires_at_ms,
                },
            )?;
            let secret = load_google_drive_secret(&app_data_dir, &provisional_library)?;
            initialize_or_sync_google_drive_library(&app_data_dir, &provisional_library, &secret)?
        }
        RemoteLibraryProvider::WebDav => {
            let webdav = webdav.ok_or_else(|| {
                library_error("missing WebDAV session details during registration".to_owned())
            })?;
            store_remote_secret(
                &app_data_dir,
                WebDavSecret {
                    library_id: library_id.clone(),
                    server_url: webdav.server_url,
                    root_url: remote_root_locator.clone(),
                    username: webdav.username,
                    password: webdav.password,
                },
            )?;
            let secret = load_webdav_secret(&app_data_dir, &provisional_library)?;
            initialize_or_sync_webdav_library(&app_data_dir, &provisional_library, &secret)?
        }
        _ => {
            return Err(library_error(
                "Only Google Drive and WebDAV remote libraries can be registered in this build"
                    .to_owned(),
            ))
        }
    };
    let library = RegisteredLibrary::remote(
        library_id.clone(),
        display_name.clone(),
        provider,
        account_id,
        remote_root_locator.clone(),
        remote_path_display,
        Some(library_root.database_path().display().to_string()),
        remote_revision.or_else(|| Some(current_unix_time_ms().to_string())),
        None,
    );
    let mut config = load_app_config(&app_data_dir)?;

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
    persist_app_config(&app_data_dir, &config)?;

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

    Ok(crate::commands::library_setup::LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

#[tauri::command]
pub fn set_remote_mirror(
    app_handle: AppHandle,
    local_library_id: String,
    remote_library_id: Option<String>,
) -> CommandResult<crate::commands::library_setup::LibraryRegistrySnapshot> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| library_error(error.to_string()))?;
    let mut config = load_app_config(&app_data_dir)?;

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

    if let Some(remote_library_id) = remote_library_id {
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
    } else {
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
    }

    persist_app_config(&app_data_dir, &config)?;

    Ok(crate::commands::library_setup::LibraryRegistrySnapshot {
        active_library_id: config.active_library_id.clone(),
        libraries: config.libraries.clone(),
    })
}

#[tauri::command]
pub fn sync_active_remote_library(state: State<'_, AppState>) -> CommandResult<()> {
    let config = load_app_config(&state.app_data_dir)?;
    let Some(active_library) = config.active_library() else {
        return Err(library_error("no library is currently active"));
    };

    if matches!(active_library, RegisteredLibrary::Remote { .. }) {
        let revision = match active_library.provider() {
            Some(RemoteLibraryProvider::WebDav) => {
                let secret = load_webdav_secret(&state.app_data_dir, active_library)?;
                initialize_or_sync_webdav_library(&state.app_data_dir, active_library, &secret)?
            }
            Some(RemoteLibraryProvider::GoogleDrive) => {
                let secret = load_google_drive_secret(&state.app_data_dir, active_library)?;
                initialize_or_sync_google_drive_library(&state.app_data_dir, active_library, &secret)?
            }
            _ => {
                return Err(library_error(
                    "the active remote provider is not supported for sync".to_owned(),
                ));
            }
        };
        update_remote_revision_in_config(&state.app_data_dir, active_library.id(), revision)?;
    }

    Ok(())
}

#[tauri::command]
pub fn publish_song_to_remote(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<UploadStatusSnapshot> {
    publish_song_internal(&state, &app_handle, &song_id)
}

#[tauri::command]
pub fn publish_songs_to_remote(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_ids: Vec<String>,
) -> CommandResult<Vec<UploadStatusSnapshot>> {
    let mut snapshots = Vec::with_capacity(song_ids.len());
    for song_id in song_ids {
        snapshots.push(publish_song_internal(&state, &app_handle, &song_id)?);
    }
    Ok(snapshots)
}

#[tauri::command]
pub fn get_all_upload_statuses(
    state: State<'_, AppState>,
) -> CommandResult<Vec<UploadStatusSnapshot>> {
    let guard = state
        .remote_upload_statuses
        .lock()
        .map_err(|_| state_lock_error("remote upload status lock was poisoned"))?;
    Ok(guard.values().cloned().collect())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_google_drive_payload_with_optional_secret() {
        let payload = serde_json::json!({
            "type": "google_drive",
            "client_id": "client-123.apps.googleusercontent.com",
            "client_secret": "secret-456"
        });

        let parsed = parse_google_drive_payload(Some(payload)).expect("payload should parse");
        assert_eq!(parsed.client_id, "client-123.apps.googleusercontent.com");
        assert_eq!(parsed.client_secret.as_deref(), Some("secret-456"));
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
        assert_eq!(query.get("access_type"), Some(&"offline".to_owned()));
        assert_eq!(query.get("prompt"), Some(&"consent".to_owned()));
        assert_eq!(query.get("code_challenge_method"), Some(&"S256".to_owned()));
        assert_eq!(query.get("state"), Some(&session.state_token));
    }

    #[test]
    fn session_ids_include_the_provider_name() {
        let session_id = session_id_for_provider(RemoteLibraryProvider::GoogleDrive);
        assert!(session_id.starts_with("remote-auth-google_drive-"));
    }

    #[test]
    fn remote_library_ids_are_stable_for_the_same_binding() {
        let first = remote_library_id(RemoteLibraryProvider::Dropbox, "acct-1", "root-1");
        let second = remote_library_id(RemoteLibraryProvider::Dropbox, "acct-1", "root-1");
        assert_eq!(first, second);
    }

    #[test]
    fn upload_status_snapshot_serializes_with_optional_remote_library() {
        let snapshot = UploadStatusSnapshot {
            song_id: "song-1".to_owned(),
            state: UploadState::Running,
            percent: 42,
            remote_library_id: Some("library-remote-1".to_owned()),
            detail: Some("Copying stems".to_owned()),
            error: None,
        };

        let json = serde_json::to_value(&snapshot).expect("snapshot should serialize");
        assert_eq!(json["song_id"], "song-1");
        assert_eq!(json["state"], "running");
        assert_eq!(json["remote_library_id"], "library-remote-1");
    }
}
