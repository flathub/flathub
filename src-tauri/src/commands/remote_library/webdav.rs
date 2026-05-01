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

    let server_segments = non_empty_path_segments(&server);
    let target_segments = non_empty_path_segments(&target);
    if !target_segments.starts_with(&server_segments) {
        return Err(library_error(format!(
            "WebDAV target URL {target_url} is not inside server URL {server_url}"
        )));
    }

    let mut current_segments = server_segments;
    for segment in target_segments.iter().skip(current_segments.len()) {
        current_segments.push(segment.clone());
        let next_path = format!("/{}/", current_segments.join("/"));
        let mut current = server.clone();
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

fn non_empty_path_segments(url: &Url) -> Vec<String> {
    url.path_segments()
        .map(|segments| {
            segments
                .filter(|segment| !segment.is_empty())
                .map(str::to_owned)
                .collect()
        })
        .unwrap_or_default()
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
        .ok_or_else(|| library_error("remote repository is missing a remote locator"))?
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
        "missing stored credentials for the remote repository".to_owned(),
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
        .ok_or_else(|| library_error("remote repository is missing a cached working copy"))?;
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
            b"openkara remote repository\n".to_vec(),
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

pub(crate) fn refresh_existing_webdav_library(
    _app_data_dir: &Path,
    library: &RegisteredLibrary,
    secret: &WebDavSecret,
) -> CommandResult<Option<String>> {
    let root_path = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote repository is missing a cached working copy"))?;
    let root = if root_path.join(".openkara-library").exists() {
        LibraryRoot::open(&root_path).map_err(library_error)?
    } else {
        LibraryRoot::create(&root_path).map_err(library_error)?
    };
    cache::initialize_library_database(&root.database_path()).map_err(library_error)?;

    let client = webdav_client()?;
    let marker_url = webdav_marker_url(&secret.root_url)?;
    if !webdav_exists(&client, &marker_url, &secret.username, &secret.password)? {
        return Err(library_error(
            "The selected WebDAV path is not an OpenKara remote repository.".to_owned(),
        ));
    }

    let database_url = webdav_database_url(&secret.root_url)?;
    download_webdav_file(
        &client,
        &database_url,
        &root.database_path(),
        &secret.username,
        &secret.password,
    )?
    .ok_or_else(|| library_error("The selected WebDAV path is missing openkara.db.".to_owned()))
    .map(Some)
}

pub(crate) fn upload_relative_file_to_remote(
    library: &RegisteredLibrary,
    secret: &WebDavSecret,
    relative_path: &str,
) -> CommandResult<()> {
    let local_root = library
        .working_copy_root()
        .ok_or_else(|| library_error("remote repository is missing a cached working copy"))?;
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
        .ok_or_else(|| library_error("remote repository is missing a cached working copy"))?;
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

pub(crate) fn delete_remote_root(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<()> {
    let secret = load_webdav_secret(app_data_dir, library)?;
    delete_relative_path_from_remote(&secret, "")
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        cache,
        config::{RemoteLibraryConnectionConfig, RemoteLibraryProvider},
        library::Song,
    };
    use std::{
        collections::{HashMap, HashSet},
        net::{Ipv4Addr, SocketAddrV4, TcpListener},
        sync::{Arc, Mutex},
        thread::{self, JoinHandle},
    };
    use tempfile::tempdir;
    use tiny_http::{Header, Method as HttpMethod, Response, Server, StatusCode as HttpStatusCode};

    struct TestWebDavServer {
        base_url: String,
        directories: Arc<Mutex<HashSet<String>>>,
        files: Arc<Mutex<HashMap<String, Vec<u8>>>>,
        server: Option<Arc<Server>>,
        thread: Option<JoinHandle<()>>,
    }

    impl TestWebDavServer {
        fn start() -> Self {
            let listener =
                TcpListener::bind(SocketAddrV4::new(Ipv4Addr::LOCALHOST, 0)).unwrap();
            let address = listener.local_addr().unwrap();
            let server = Arc::new(Server::from_listener(listener, None).unwrap());
            let directories = Arc::new(Mutex::new(HashSet::from(["/".to_owned()])));
            let files = Arc::new(Mutex::new(HashMap::new()));
            let thread_directories = Arc::clone(&directories);
            let thread_files = Arc::clone(&files);
            let thread_server = Arc::clone(&server);
            let thread = thread::spawn(move || {
                while let Ok(request) = thread_server.recv() {
                    respond_to_webdav_request(request, &thread_directories, &thread_files);
                }
            });

            Self {
                base_url: format!("http://127.0.0.1:{}/", address.port()),
                directories,
                files,
                server: Some(server),
                thread: Some(thread),
            }
        }

        fn directory_exists(&self, path: &str) -> bool {
            self.directories.lock().unwrap().contains(path)
        }

        fn file(&self, path: &str) -> Option<Vec<u8>> {
            self.files.lock().unwrap().get(path).cloned()
        }
    }

    impl Drop for TestWebDavServer {
        fn drop(&mut self) {
            if let Some(server) = self.server.take() {
                server.unblock();
            }
            if let Some(thread) = self.thread.take() {
                thread.join().unwrap();
            }
        }
    }

    fn respond_to_webdav_request(
        mut request: tiny_http::Request,
        directories: &Arc<Mutex<HashSet<String>>>,
        files: &Arc<Mutex<HashMap<String, Vec<u8>>>>,
    ) {
        let path = request.url().split('?').next().unwrap_or("/").to_owned();
        match request.method() {
            &HttpMethod::Head => {
                let exists = if path.ends_with('/') {
                    directories.lock().unwrap().contains(&path)
                } else {
                    files.lock().unwrap().contains_key(&path)
                };
                let status = if exists { 204 } else { 404 };
                let _ = request.respond(Response::empty(HttpStatusCode(status)));
            }
            &HttpMethod::Put => {
                let mut body = Vec::new();
                request.as_reader().read_to_end(&mut body).unwrap();
                files.lock().unwrap().insert(path, body);
                let mut response = Response::empty(HttpStatusCode(201));
                response.add_header(Header::from_bytes("ETag", b"test-etag").unwrap());
                let _ = request.respond(response);
            }
            &HttpMethod::Get => {
                let body = files.lock().unwrap().get(&path).cloned();
                match body {
                    Some(body) => {
                        let mut response =
                            Response::from_data(body).with_status_code(HttpStatusCode(200));
                        response.add_header(Header::from_bytes("ETag", b"test-etag").unwrap());
                        let _ = request.respond(response);
                    }
                    None => {
                        let _ = request.respond(Response::empty(HttpStatusCode(404)));
                    }
                }
            }
            &HttpMethod::NonStandard(ref method) if method.as_str() == "MKCOL" => {
                directories.lock().unwrap().insert(path);
                let _ = request.respond(Response::empty(HttpStatusCode(201)));
            }
            _ => {
                let _ = request.respond(Response::empty(HttpStatusCode(405)));
            }
        }
    }

    fn test_remote_library(root_path: &Path, server_url: &str, root_url: &str) -> RegisteredLibrary {
        RegisteredLibrary::remote(
            "remote-webdav-test".to_owned(),
            "Remote WebDAV Test".to_owned(),
            RemoteLibraryProvider::WebDav,
            "openkara".to_owned(),
            root_url.to_owned(),
            "127.0.0.1/OpenKara".to_owned(),
            Some(RemoteLibraryConnectionConfig::WebDav {
                server_url: server_url.to_owned(),
            }),
            Some(root_path.join("openkara.db").to_string_lossy().into_owned()),
            None,
        )
    }

    #[test]
    fn webdav_initializes_uploads_and_reopens_remote_library() {
        let server = TestWebDavServer::start();
        let app_data_dir = tempdir().unwrap();
        let first_working_copy = tempdir().unwrap();
        let root_url = join_url(&server.base_url, "OpenKara/").unwrap();
        let secret = WebDavSecret {
            root_url: root_url.clone(),
            username: "openkara".to_owned(),
            password: "secret".to_owned(),
        };
        let first_library =
            test_remote_library(first_working_copy.path(), &server.base_url, &root_url);

        initialize_or_sync_webdav_library(app_data_dir.path(), &first_library, &secret)
            .expect("new WebDAV remote repository should initialize");
        let local_root = LibraryRoot::open(first_working_copy.path()).unwrap();
        let media_path = local_root.media_path("song-1", "wav");
        fs::write(&media_path, b"openkara test audio").unwrap();
        let connection = cache::open_database(&local_root.database_path()).unwrap();
        cache::upsert_song(
            &connection,
            &Song {
                hash: "song-1".to_owned(),
                file_path: Some("media/song-1.wav".to_owned()),
                cdg_path: None,
                media_g_container: None,
                instrumental: true,
                audio_source_kind: "original".to_owned(),
                title: Some("Remote Song".to_owned()),
                artist: Some("OpenKara".to_owned()),
                album: None,
                duration_ms: 1_000,
                cover_art: None,
                imported_at: 1,
                original_ext: Some("wav".to_owned()),
            },
        )
        .unwrap();

        upload_relative_file_to_remote(&first_library, &secret, "media/song-1.wav")
            .expect("media file should upload");
        upload_relative_file_to_remote(&first_library, &secret, "openkara.db")
            .expect("library metadata should upload");

        assert!(server.directory_exists("/OpenKara/"));
        assert!(server.directory_exists("/OpenKara/media/"));
        assert!(server.directory_exists("/OpenKara/media-g/"));
        assert!(server.directory_exists("/OpenKara/stems/"));
        assert_eq!(
            server.file("/OpenKara/media/song-1.wav").as_deref(),
            Some(b"openkara test audio".as_slice())
        );
        assert!(server.file("/OpenKara/openkara.db").is_some());

        let second_working_copy = tempdir().unwrap();
        let second_library =
            test_remote_library(second_working_copy.path(), &server.base_url, &root_url);
        initialize_or_sync_webdav_library(app_data_dir.path(), &second_library, &secret)
            .expect("existing WebDAV remote repository should reopen");
        let second_root = LibraryRoot::open(second_working_copy.path()).unwrap();
        let second_connection = cache::open_database(&second_root.database_path()).unwrap();
        let songs = cache::list_songs(&second_connection).unwrap();

        assert_eq!(songs.len(), 1);
        assert_eq!(songs[0].title.as_deref(), Some("Remote Song"));
        assert_eq!(songs[0].file_path.as_deref(), Some("media/song-1.wav"));
    }

    #[test]
    fn webdav_refresh_existing_rejects_empty_remote_location() {
        let server = TestWebDavServer::start();
        let app_data_dir = tempdir().unwrap();
        let working_copy = tempdir().unwrap();
        let root_url = join_url(&server.base_url, "MovedOpenKara/").unwrap();
        let secret = WebDavSecret {
            root_url: root_url.clone(),
            username: "openkara".to_owned(),
            password: "secret".to_owned(),
        };
        let library = test_remote_library(working_copy.path(), &server.base_url, &root_url);

        let error = refresh_existing_webdav_library(app_data_dir.path(), &library, &secret)
            .expect_err("empty WebDAV path should not be initialized during relocation");

        assert!(error
            .message
            .contains("not an OpenKara remote repository"));
        assert!(!server.directory_exists("/MovedOpenKara/"));
        assert!(server.file("/MovedOpenKara/openkara.db").is_none());
    }
}
