use crate::{
    cache,
    commands::error::{library_error, CommandResult},
    config::RegisteredLibrary,
    library_root::LibraryRoot,
};
use reqwest::{
    blocking::{Client, Response},
    header::ETAG,
    Method, StatusCode, Url,
};
use std::{
    fs,
    io::Write,
    path::Path,
};

use super::types::{
    load_remote_credential, slugify_display_name, store_remote_credential, stored_webdav_server_url,
    RemoteAuthPayloadInput, StoredWebDavSecret, WebDavSecret, WebDavSessionData,
};

pub(crate) fn normalize_server_url(raw: &str) -> CommandResult<String> {
    let mut url = Url::parse(raw)
        .map_err(|error| library_error(format!("invalid WebDAV server URL: {error}")))?;
    if !raw.ends_with('/') {
        let next_path = format!("{}/", url.path().trim_end_matches('/'));
        url.set_path(&next_path);
    }
    Ok(url.to_string())
}

pub(crate) fn normalize_webdav_root_path(raw: Option<&str>, fallback_display_name: &str) -> String {
    let candidate = raw.unwrap_or_default().trim().trim_matches('/');
    if candidate.is_empty() {
        format!("/{}", slugify_display_name(fallback_display_name))
    } else {
        format!("/{}", candidate)
    }
}

pub(crate) fn join_url(base: &str, relative: &str) -> CommandResult<String> {
    Url::parse(base)
        .and_then(|url| url.join(relative))
        .map(|url| url.to_string())
        .map_err(|error| library_error(format!("failed to join URL {base} + {relative}: {error}")))
}

pub(crate) fn remote_path_display_from_url(url: &str) -> String {
    Url::parse(url)
        .ok()
        .map(|url| {
            let host = url.host_str().unwrap_or("webdav");
            let path = url.path().trim_end_matches('/');
            format!("{host}{path}")
        })
        .unwrap_or_else(|| url.to_owned())
}

pub(crate) fn webdav_client() -> CommandResult<Client> {
    Client::builder()
        .redirect(reqwest::redirect::Policy::limited(10))
        .build()
        .map_err(|error| library_error(format!("failed to create WebDAV client: {error}")))
}

pub(crate) fn webdav_send(
    client: &Client,
    method: Method,
    url: &str,
    username: &str,
    password: &str,
    if_match: Option<&str>,
    body: Option<Vec<u8>>,
) -> CommandResult<Response> {
    let mut request = client.request(method, url).basic_auth(username, Some(password));
    if let Some(tag) = if_match {
        request = request.header("If-Match", tag);
    }
    if let Some(bytes) = body {
        request = request.body(bytes);
    }
    request
        .send()
        .map_err(|error| library_error(format!("WebDAV request to {url} failed: {error}")))
}

pub(crate) fn webdav_exists(
    client: &Client,
    url: &str,
    username: &str,
    password: &str,
) -> CommandResult<bool> {
    Ok(webdav_send(client, Method::HEAD, url, username, password, None, None)?.status()
        != StatusCode::NOT_FOUND)
}

pub(crate) fn webdav_get_etag(
    client: &Client,
    url: &str,
    username: &str,
    password: &str,
) -> CommandResult<Option<String>> {
    let response = webdav_send(client, Method::HEAD, url, username, password, None, None)?;
    if response.status() == StatusCode::NOT_FOUND {
        return Ok(None);
    }
    if !response.status().is_success() {
        return Err(library_error(format!(
            "WebDAV HEAD {url} failed with status {}",
            response.status()
        )));
    }
    Ok(response
        .headers()
        .get(ETAG)
        .and_then(|value| value.to_str().ok())
        .map(str::to_owned))
}

pub(crate) fn ensure_webdav_collection_chain(
    client: &Client,
    server_url: &str,
    target_url: &str,
    username: &str,
    password: &str,
) -> CommandResult<()> {
    let server = Url::parse(server_url)
        .map_err(|error| library_error(format!("invalid WebDAV server URL: {error}")))?;
    let target = Url::parse(target_url)
        .map_err(|error| library_error(format!("invalid WebDAV target URL: {error}")))?;

    let server_segments: Vec<String> = server
        .path_segments()
        .map(|segments| segments.map(str::to_owned).collect())
        .unwrap_or_default();
    let target_segments: Vec<String> = target
        .path_segments()
        .map(|segments| segments.map(str::to_owned).collect())
        .unwrap_or_default();

    let mut current = server.clone();
    for segment in target_segments.iter().skip(server_segments.len()) {
        let next_path = format!("{}{segment}/", current.path());
        current.set_path(&next_path);
        let current_url = current.to_string();
        if webdav_exists(client, &current_url, username, password)? {
            continue;
        }

        let response = webdav_send(
            client,
            Method::from_bytes(b"MKCOL").expect("MKCOL should parse"),
            &current_url,
            username,
            password,
            None,
            None,
        )?;
        match response.status() {
            StatusCode::CREATED | StatusCode::METHOD_NOT_ALLOWED | StatusCode::CONFLICT => {}
            status => {
                return Err(library_error(format!(
                    "failed to create WebDAV collection {current_url}: {status}"
                )))
            }
        }
    }
    Ok(())
}

pub(crate) fn download_webdav_file(
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
            "failed to download {url}: {}",
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
        .map_err(|error| library_error(format!("failed to read WebDAV response: {error}")))?;
    let mut file = fs::File::create(destination).map_err(|error| {
        library_error(format!("failed to create {}: {error}", destination.display()))
    })?;
    file.write_all(bytes.as_ref()).map_err(|error| {
        library_error(format!("failed to write {}: {error}", destination.display()))
    })?;
    Ok(etag)
}

pub(crate) fn upload_webdav_file(
    client: &Client,
    url: &str,
    source: &Path,
    username: &str,
    password: &str,
) -> CommandResult<Option<String>> {
    let bytes = fs::read(source)
        .map_err(|error| library_error(format!("failed to read {}: {error}", source.display())))?;
    upload_webdav_bytes(client, url, bytes, username, password)
}

pub(crate) fn upload_webdav_bytes(
    client: &Client,
    url: &str,
    bytes: Vec<u8>,
    username: &str,
    password: &str,
) -> CommandResult<Option<String>> {
    let response = webdav_send(client, Method::PUT, url, username, password, None, Some(bytes))?;
    if !response.status().is_success() {
        return Err(library_error(format!(
            "failed to upload {url}: {}",
            response.status()
        )));
    }
    Ok(response
        .headers()
        .get(ETAG)
        .and_then(|value| value.to_str().ok())
        .map(str::to_owned))
}

pub(crate) fn parse_webdav_payload(
    payload: Option<serde_json::Value>,
) -> CommandResult<WebDavSessionData> {
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
    }
}

pub(crate) fn store_webdav_secret(
    app_data_dir: &Path,
    library_id: &str,
    username: String,
    password: String,
) -> CommandResult<()> {
    store_remote_credential(
        app_data_dir,
        library_id,
        &StoredWebDavSecret { username, password },
    )
}

pub(crate) fn load_webdav_secret(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<WebDavSecret> {
    let remote_root_url = library
        .remote_root_locator()
        .ok_or_else(|| library_error("remote library is missing a remote locator"))?
        .to_owned();
    if let Some(secret) = load_remote_credential::<StoredWebDavSecret>(app_data_dir, library.id())?
    {
        return Ok(WebDavSecret {
            root_url: remote_root_url,
            username: secret.username,
            password: secret.password,
        });
    }
    Err(library_error(
        "missing stored credentials for the remote library".to_owned(),
    ))
}

pub(crate) fn webdav_marker_url(root_url: &str) -> CommandResult<String> {
    join_url(root_url, ".openkara-library")
}

pub(crate) fn webdav_database_url(root_url: &str) -> CommandResult<String> {
    join_url(root_url, "openkara.db")
}

pub(crate) fn initialize_or_sync_webdav_library(
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
    let server_url = stored_webdav_server_url(library)?;
    ensure_webdav_collection_chain(
        &client,
        &server_url,
        &secret.root_url,
        &secret.username,
        &secret.password,
    )?;

    for directory in ["media", "media-g", "stems"] {
        let directory_url = join_url(&secret.root_url, &format!("{directory}/"))?;
        ensure_webdav_collection_chain(
            &client,
            &server_url,
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

pub(crate) fn upload_relative_file_to_remote(
    library: &RegisteredLibrary,
    secret: &WebDavSecret,
    relative_path: &str,
) -> CommandResult<()> {
    let local_root = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote library is missing a cached working copy"))?;
    let source = local_root.join(relative_path);
    let client = webdav_client()?;
    let server_url = stored_webdav_server_url(library)?;
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
                &server_url,
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

pub(crate) fn upload_directory_to_remote(
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

pub(crate) fn delete_relative_path_from_remote(
    secret: &WebDavSecret,
    relative_path: &str,
) -> CommandResult<()> {
    let client = webdav_client()?;
    let url = join_url(&secret.root_url, relative_path)?;
    let response = webdav_send(
        &client,
        Method::DELETE,
        &url,
        &secret.username,
        &secret.password,
        None,
        None,
    )?;
    match response.status() {
        StatusCode::OK
        | StatusCode::NO_CONTENT
        | StatusCode::ACCEPTED
        | StatusCode::NOT_FOUND => Ok(()),
        status => Err(library_error(format!("failed to delete {url}: {status}"))),
    }
}
