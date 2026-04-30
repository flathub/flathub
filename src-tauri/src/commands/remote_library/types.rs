use crate::{
    cache,
    commands::error::{database_error, library_error, CommandError, CommandResult},
    config::{self, AppConfig, RegisteredLibrary, RemoteLibraryProvider},
    library_root::LibraryRoot,
    system_credentials,
};
use serde::{Deserialize, Serialize};
use std::{
    path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
};

pub(crate) const REMOTE_LIBRARIES_DIR: &str = "remote-libraries";
pub(crate) const GOOGLE_DRIVE_CLIENT_ID_ENV: &str = "OPENKARA_GOOGLE_DRIVE_CLIENT_ID";
pub(crate) const GOOGLE_DRIVE_CLIENT_SECRET_ENV: &str = "OPENKARA_GOOGLE_DRIVE_CLIENT_SECRET";
pub(crate) const GOOGLE_DRIVE_OAUTH_CLIENT_RESOURCE_PATH: &str =
    "oauth/google-drive-client.json";
pub(crate) const DROPBOX_APP_KEY_ENV: &str = "OPENKARA_DROPBOX_APP_KEY";
pub(crate) const DROPBOX_APP_SECRET_ENV: &str = "OPENKARA_DROPBOX_APP_SECRET";
pub(crate) const DROPBOX_OAUTH_CLIENT_RESOURCE_PATH: &str = "oauth/dropbox-client.json";
pub(crate) const DROPBOX_FIXED_REDIRECT_PORT: u16 = 53_682;
pub(crate) const DROPBOX_FIXED_REDIRECT_URI: &str = "http://localhost:53682/oauth2/callback";
pub(crate) const GOOGLE_DRIVE_OAUTH_SCOPE: &str =
    "openid email https://www.googleapis.com/auth/drive.file";

#[derive(Debug, Clone, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub(crate) enum RemoteAuthPayloadInput {
    WebDav {
        server_url: String,
        username: String,
        password: String,
        root_path: Option<String>,
    },
}

#[derive(Debug, Clone)]
pub(crate) struct WebDavSessionData {
    pub(crate) server_url: String,
    pub(crate) username: String,
    pub(crate) password: String,
    pub(crate) root_path: Option<String>,
}

#[derive(Debug, Clone)]
pub(crate) struct GoogleDriveSessionData {
    pub(crate) client_id: String,
    pub(crate) client_secret: Option<String>,
    pub(crate) code_verifier: String,
    pub(crate) redirect_uri: String,
    pub(crate) state_token: String,
    pub(crate) root_folder_id: Option<String>,
    pub(crate) access_token: Option<String>,
    pub(crate) refresh_token: Option<String>,
    pub(crate) access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone)]
pub(crate) struct DropboxSessionData {
    pub(crate) app_key: String,
    pub(crate) app_secret: Option<String>,
    pub(crate) code_verifier: String,
    pub(crate) redirect_uri: String,
    pub(crate) state_token: String,
    pub(crate) access_token: Option<String>,
    pub(crate) refresh_token: Option<String>,
    pub(crate) access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone)]
pub(crate) struct WebDavSecret {
    pub(crate) root_url: String,
    pub(crate) username: String,
    pub(crate) password: String,
}

#[derive(Debug, Clone)]
pub(crate) struct GoogleDriveSecret {
    pub(crate) library_id: String,
    pub(crate) client_id: String,
    pub(crate) client_secret: Option<String>,
    pub(crate) access_token: String,
    pub(crate) refresh_token: String,
    pub(crate) access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone)]
pub(crate) struct DropboxSecret {
    pub(crate) library_id: String,
    pub(crate) app_key: String,
    pub(crate) app_secret: Option<String>,
    pub(crate) access_token: String,
    pub(crate) refresh_token: String,
    pub(crate) access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct StoredGoogleDriveSecret {
    pub(crate) client_secret: Option<String>,
    pub(crate) access_token: String,
    pub(crate) refresh_token: String,
    pub(crate) access_token_expires_at_ms: Option<i64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct StoredDropboxSecret {
    pub(crate) refresh_token: String,
    pub(crate) access_token: String,
    pub(crate) access_token_expires_at_ms: Option<i64>,
    pub(crate) app_secret: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct StoredWebDavSecret {
    pub(crate) username: String,
    pub(crate) password: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct BundledGoogleDriveOAuthClientFile {
    pub(crate) installed: BundledGoogleDriveInstalledClient,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct BundledDropboxOAuthClientFile {
    pub(crate) app_key: String,
    #[serde(default)]
    pub(crate) app_secret: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct BundledGoogleDriveInstalledClient {
    pub(crate) client_id: String,
    #[serde(default)]
    pub(crate) client_secret: Option<String>,
}

#[derive(Debug, Clone)]
pub(crate) struct GoogleDriveProviderCredentials {
    pub(crate) client_id: String,
    pub(crate) client_secret: Option<String>,
}

#[derive(Debug, Clone)]
pub(crate) struct DropboxProviderCredentials {
    pub(crate) app_key: String,
    pub(crate) app_secret: Option<String>,
}

#[derive(Debug, Deserialize)]
pub(crate) struct GoogleDriveTokenResponse {
    pub(crate) access_token: String,
    pub(crate) expires_in: Option<i64>,
    pub(crate) refresh_token: Option<String>,
}

#[derive(Debug, Deserialize)]
pub(crate) struct DropboxTokenResponse {
    pub(crate) access_token: String,
    pub(crate) expires_in: Option<i64>,
    pub(crate) refresh_token: Option<String>,
    pub(crate) account_id: Option<String>,
}

#[derive(Debug, Deserialize)]
pub(crate) struct GoogleDriveUserInfoResponse {
    pub(crate) sub: String,
    #[serde(default)]
    pub(crate) email: Option<String>,
}

#[derive(Debug, Clone, Deserialize)]
pub(crate) struct GoogleDriveFileMetadata {
    pub(crate) id: String,
    #[serde(rename = "headRevisionId")]
    pub(crate) head_revision_id: Option<String>,
    #[serde(default, rename = "modifiedTime")]
    pub(crate) modified_time: Option<String>,
}

#[derive(Debug, Deserialize)]
pub(crate) struct GoogleDriveFileListResponse {
    #[serde(default)]
    pub(crate) files: Vec<GoogleDriveFileMetadata>,
}

#[derive(Debug, Clone, Deserialize)]
pub(crate) struct DropboxMetadata {
    #[serde(default)]
    pub(crate) rev: Option<String>,
    #[serde(default)]
    pub(crate) server_modified: Option<String>,
}

#[derive(Debug, Deserialize)]
pub(crate) struct DropboxCreateFolderResponse {
    pub(crate) metadata: DropboxMetadata,
}

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
    pub(crate) google_drive: Option<GoogleDriveSessionData>,
    pub(crate) dropbox: Option<DropboxSessionData>,
    pub(crate) webdav: Option<WebDavSessionData>,
}

#[derive(Debug, Clone, Serialize)]
pub(crate) struct UploadProgressPayload {
    pub(crate) song_id: String,
    pub(crate) percent: u8,
    pub(crate) remote_library_id: Option<String>,
    pub(crate) detail: Option<String>,
}

#[derive(Debug, Clone, Serialize)]
pub(crate) struct UploadCompletePayload {
    pub(crate) song_id: String,
    pub(crate) remote_library_id: Option<String>,
}

#[derive(Debug, Clone, Serialize)]
pub(crate) struct UploadErrorPayload {
    pub(crate) song_id: String,
    pub(crate) remote_library_id: Option<String>,
    pub(crate) error: CommandError,
}

pub(crate) fn current_unix_time_ms() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_millis() as i64
}

pub(crate) fn session_id_for_provider(provider: RemoteLibraryProvider) -> String {
    format!(
        "remote-auth-{}-{}",
        provider.as_str(),
        current_unix_time_ms()
    )
}

pub(crate) fn remote_library_id(
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

pub(crate) fn remote_libraries_dir(app_data_dir: &Path) -> PathBuf {
    app_data_dir.join(REMOTE_LIBRARIES_DIR)
}

pub(crate) fn remote_library_root(app_data_dir: &Path, library_id: &str) -> PathBuf {
    remote_libraries_dir(app_data_dir).join(library_id)
}

pub(crate) fn store_remote_credential<T: Serialize>(
    app_data_dir: &Path,
    library_id: &str,
    secret: &T,
) -> CommandResult<()> {
    system_credentials::store_json(app_data_dir, library_id, secret).map_err(|error| {
        library_error(format!(
            "failed to store remote credentials in the system credential store: {error}"
        ))
    })
}

pub(crate) fn load_remote_credential<T: serde::de::DeserializeOwned>(
    app_data_dir: &Path,
    library_id: &str,
) -> CommandResult<Option<T>> {
    system_credentials::load_json(app_data_dir, library_id).map_err(|error| {
        library_error(format!(
            "failed to load remote credentials from the system credential store: {error}"
        ))
    })
}

pub(crate) fn delete_remote_credential(app_data_dir: &Path, library_id: &str) -> CommandResult<()> {
    system_credentials::delete(app_data_dir, library_id).map_err(|error| {
        library_error(format!(
            "failed to remove remote credentials from the system credential store: {error}"
        ))
    })
}

pub(crate) fn stored_google_drive_client_id(library: &RegisteredLibrary) -> CommandResult<String> {
    library
        .google_drive_client_id()
        .map(str::to_owned)
        .ok_or_else(|| {
            library_error(
                "remote library is missing the Google Drive OAuth client ID metadata".to_owned(),
            )
        })
}

pub(crate) fn stored_dropbox_app_key(library: &RegisteredLibrary) -> CommandResult<String> {
    library.dropbox_app_key().map(str::to_owned).ok_or_else(|| {
        library_error("remote library is missing the Dropbox app key metadata".to_owned())
    })
}

pub(crate) fn stored_webdav_server_url(library: &RegisteredLibrary) -> CommandResult<String> {
    library.webdav_server_url().map(str::to_owned).ok_or_else(|| {
        library_error("remote library is missing the WebDAV server URL metadata".to_owned())
    })
}

pub(crate) fn load_app_config(app_data_dir: &Path) -> CommandResult<AppConfig> {
    Ok(config::load_config(app_data_dir)
        .map_err(library_error)?
        .unwrap_or_default())
}

pub(crate) fn persist_app_config(app_data_dir: &Path, config: &AppConfig) -> CommandResult<()> {
    config::save_config(app_data_dir, config).map_err(library_error)
}

pub(crate) fn load_remote_root(
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

pub(crate) fn slugify_display_name(display_name: &str) -> String {
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

pub(crate) fn upsert_stem_entry(
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::RemoteLibraryProvider;

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
