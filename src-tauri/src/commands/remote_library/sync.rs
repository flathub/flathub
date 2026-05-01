use crate::{
    cache,
    commands::error::{
        database_error, library_error, state_lock_error, CommandError, CommandResult,
    },
    config::{AppConfig, RegisteredLibrary, RemoteLibraryProvider},
    library::Song,
    library_root::LibraryRoot,
    AppState,
};
use std::{
    collections::HashMap,
    fs,
    path::Path,
};
use tauri::{AppHandle, Emitter};

use super::{
    dropbox::{
        delete_relative_path_from_remote as dropbox_delete_relative_path_from_remote,
        dropbox_download_file, dropbox_get_metadata, dropbox_join_path, dropbox_metadata_revision,
        dropbox_upload_directory_to_remote, dropbox_upload_relative_file_to_remote,
        initialize_or_sync_dropbox_library, load_dropbox_secret,
    },
    google_drive::{
        delete_relative_path_from_remote as google_drive_delete_relative_path_from_remote,
        google_drive_download_file, google_drive_find_relative_entry,
        google_drive_upload_directory_to_remote, google_drive_upload_relative_file_to_remote,
        initialize_or_sync_google_drive_library, load_google_drive_secret,
    },
    types::{
        current_unix_time_ms, load_app_config, load_remote_root, persist_app_config,
        upsert_stem_entry, UploadCompletePayload, UploadErrorPayload, UploadProgressPayload,
        UploadState, UploadStatusSnapshot,
    },
    webdav::{
        delete_relative_path_from_remote as webdav_delete_relative_path_from_remote,
        download_webdav_file, initialize_or_sync_webdav_library, join_url, load_webdav_secret,
        upload_directory_to_remote, upload_relative_file_to_remote, webdav_client,
        webdav_database_url, webdav_get_etag,
    },
};

fn mark_upload_status(
    state: &AppState,
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

fn emit_upload_progress<R: tauri::Runtime>(app_handle: &AppHandle<R>, snapshot: &UploadStatusSnapshot) {
    let payload = UploadProgressPayload {
        song_id: snapshot.song_id.clone(),
        percent: snapshot.percent,
        remote_library_id: snapshot.remote_library_id.clone(),
        detail: snapshot.detail.clone(),
    };
    let _ = app_handle.emit("upload-progress", payload);
}

fn emit_upload_complete<R: tauri::Runtime>(app_handle: &AppHandle<R>, snapshot: &UploadStatusSnapshot) {
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

fn load_registered_remote_library(
    app_data_dir: &Path,
    library_id: &str,
) -> CommandResult<RegisteredLibrary> {
    let config = load_app_config(app_data_dir)?;
    let library = config
        .libraries
        .iter()
        .find(|entry| entry.id() == library_id)
        .ok_or_else(|| library_error(format!("remote repository {library_id} was not found")))?;
    if !matches!(library, RegisteredLibrary::Remote { .. }) {
        return Err(library_error(format!(
            "library {library_id} is not a remote repository"
        )));
    }
    Ok(library.clone())
}

fn remote_database_revision(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<Option<String>> {
    match library.provider() {
        Some(RemoteLibraryProvider::WebDav) => {
            let secret = load_webdav_secret(app_data_dir, library)?;
            webdav_get_etag(
                &webdav_client()?,
                &webdav_database_url(&secret.root_url)?,
                &secret.username,
                &secret.password,
            )
        }
        Some(RemoteLibraryProvider::GoogleDrive) => {
            let mut secret = load_google_drive_secret(app_data_dir, library)?;
            let root_folder_id = library
                .remote_root_locator()
                .ok_or_else(|| library_error("remote repository is missing a remote locator".to_owned()))?;
            Ok(google_drive_find_relative_entry(
                app_data_dir,
                &mut secret,
                root_folder_id,
                "openkara.db",
            )?
            .and_then(|metadata| metadata.head_revision_id.or(metadata.modified_time)))
        }
        Some(RemoteLibraryProvider::Dropbox) => {
            let mut secret = load_dropbox_secret(app_data_dir, library)?;
            let root_path = library
                .remote_root_locator()
                .ok_or_else(|| library_error("remote repository is missing a remote locator".to_owned()))?;
            Ok(dropbox_get_metadata(
                app_data_dir,
                &mut secret,
                &dropbox_join_path(root_path, "openkara.db"),
            )?
            .and_then(|metadata| dropbox_metadata_revision(&metadata)))
        }
        _ => Err(library_error(
            "the active remote provider is not supported for database revision checks".to_owned(),
        )),
    }
}

fn remote_database_revision_is_stale(
    stored_revision: Option<&str>,
    provider_revision: Option<&str>,
) -> bool {
    provider_revision.is_some_and(|revision| Some(revision) != stored_revision)
}

fn remote_database_conflict_error(provider_revision: Option<&str>) -> CommandError {
    let revision_detail = provider_revision
        .map(|revision| format!(" Remote revision: {revision}."))
        .unwrap_or_default();
    library_error(format!(
        "Remote repository database changed on another device before this publish. \
         OpenKara stopped before overwriting it. Use Settings > Karaoke Library > \
         Refresh remote repository, then retry this edit. If refresh fails because authentication \
         or the server changed, use Reauthorize remote repository first.{revision_detail}"
    ))
}

fn sync_remote_database_from_provider(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<RegisteredLibrary> {
    let revision = match library.provider() {
        Some(RemoteLibraryProvider::WebDav) => {
            let secret = load_webdav_secret(app_data_dir, library)?;
            initialize_or_sync_webdav_library(app_data_dir, library, &secret)?
        }
        Some(RemoteLibraryProvider::GoogleDrive) => {
            let secret = load_google_drive_secret(app_data_dir, library)?;
            initialize_or_sync_google_drive_library(app_data_dir, library, &secret)?
        }
        Some(RemoteLibraryProvider::Dropbox) => {
            let secret = load_dropbox_secret(app_data_dir, library)?;
            initialize_or_sync_dropbox_library(app_data_dir, library, &secret)?
        }
        None => {
            return Err(library_error(
                "the active remote provider is not supported for sync".to_owned(),
            ))
        }
    };
    update_remote_revision_in_config(app_data_dir, library.id(), revision)?;
    load_registered_remote_library(app_data_dir, library.id())
}

fn prepare_remote_database_for_mutation(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<RegisteredLibrary> {
    let provider_revision = remote_database_revision(app_data_dir, library)?;
    if remote_database_revision_is_stale(library.remote_revision(), provider_revision.as_deref()) {
        return sync_remote_database_from_provider(app_data_dir, library);
    }
    Ok(library.clone())
}

fn ensure_remote_database_upload_safe(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
) -> CommandResult<()> {
    let provider_revision = remote_database_revision(app_data_dir, library)?;
    if remote_database_revision_is_stale(library.remote_revision(), provider_revision.as_deref()) {
        return Err(remote_database_conflict_error(provider_revision.as_deref()));
    }
    Ok(())
}

fn upload_remote_database(app_data_dir: &Path, library: &RegisteredLibrary) -> CommandResult<()> {
    ensure_remote_database_upload_safe(app_data_dir, library)?;

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
                .ok_or_else(|| library_error("remote repository is missing a remote locator".to_owned()))?;
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
        Some(RemoteLibraryProvider::Dropbox) => {
            let secret = load_dropbox_secret(app_data_dir, library)?;
            let root_path = library
                .remote_root_locator()
                .ok_or_else(|| library_error("remote repository is missing a remote locator".to_owned()))?;
            dropbox_upload_relative_file_to_remote(
                app_data_dir,
                library,
                &secret,
                "openkara.db",
                root_path,
            )?;
            let mut refreshed_secret = load_dropbox_secret(app_data_dir, library)?;
            let metadata = dropbox_get_metadata(
                app_data_dir,
                &mut refreshed_secret,
                &dropbox_join_path(root_path, "openkara.db"),
            )?
            .ok_or_else(|| library_error("Dropbox database file is missing after upload".to_owned()))?;
            update_remote_revision_in_config(
                app_data_dir,
                library.id(),
                dropbox_metadata_revision(&metadata),
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

pub fn prepare_active_remote_database_for_mutation(app_data_dir: &Path) -> CommandResult<()> {
    let Some(library) = active_remote_library(app_data_dir)? else {
        return Ok(());
    };
    let _ = prepare_remote_database_for_mutation(app_data_dir, &library)?;
    Ok(())
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
                .ok_or_else(|| library_error("remote repository is missing a remote locator".to_owned()))?;
            let entry = google_drive_find_relative_entry(
                app_data_dir,
                &mut secret,
                root_folder_id,
                relative_path,
            )?
            .ok_or_else(|| library_error(format!("remote file {relative_path} was not found")))?;
            google_drive_download_file(app_data_dir, &mut secret, &entry.id, &destination)
        }
        Some(RemoteLibraryProvider::Dropbox) => {
            let mut secret = load_dropbox_secret(app_data_dir, &library)?;
            let root_path = library
                .remote_root_locator()
                .ok_or_else(|| library_error("remote repository is missing a remote locator".to_owned()))?;
            let remote_path = dropbox_join_path(root_path, relative_path);
            if dropbox_get_metadata(app_data_dir, &mut secret, &remote_path)?.is_none() {
                return Err(library_error(format!("remote file {relative_path} was not found")));
            }
            dropbox_download_file(app_data_dir, &mut secret, &remote_path, &destination)
        }
        _ => Err(library_error(
            "the active remote provider is not supported for file caching".to_owned(),
        )),
    }
}

fn resolve_active_remote(config: &AppConfig) -> Option<RegisteredLibrary> {
    config.active_library().and_then(|library| match library {
        RegisteredLibrary::Remote { .. } => Some(library.clone()),
        RegisteredLibrary::Local { .. } => None,
    })
}

fn remote_delete_relative_path(
    app_data_dir: &Path,
    library: &RegisteredLibrary,
    relative_path: &str,
) -> CommandResult<()> {
    match library.provider() {
        Some(RemoteLibraryProvider::WebDav) => {
            let secret = load_webdav_secret(app_data_dir, library)?;
            webdav_delete_relative_path_from_remote(&secret, relative_path)
        }
        Some(RemoteLibraryProvider::GoogleDrive) => {
            google_drive_delete_relative_path_from_remote(app_data_dir, library, relative_path)
        }
        Some(RemoteLibraryProvider::Dropbox) => {
            dropbox_delete_relative_path_from_remote(app_data_dir, library, relative_path)
        }
        None => Err(library_error(
            "the bound remote provider is not supported for deletion".to_owned(),
        )),
    }
}

fn song_ready_for_remote_publish(
    connection: &rusqlite::Connection,
    library_root: &LibraryRoot,
    song: &Song,
) -> CommandResult<bool> {
    if !song.is_separable() {
        return Ok(true);
    }

    let Some(entry) = cache::stems::get_cached_stem_entry(connection, &song.hash)
        .map_err(|error| database_error(error.to_string()))?
    else {
        return Ok(false);
    };

    Ok(cache::stems::cache_entry_files_valid(library_root, &entry))
}

fn desired_remote_audio_source_kind(
    connection: &rusqlite::Connection,
    library_root: &LibraryRoot,
    song: &Song,
) -> CommandResult<Option<&'static str>> {
    if !song_ready_for_remote_publish(connection, library_root, song)? {
        return Ok(None);
    }

    Ok(Some(if song.is_separable() {
        "stems_remote"
    } else {
        "original_remote"
    }))
}

fn sync_song_lyrics_to_remote(
    local_connection: &rusqlite::Connection,
    remote_connection: &rusqlite::Connection,
    song_id: &str,
) -> CommandResult<()> {
    if let Some(entry) = cache::lyrics::get_lyrics_cache_entry(local_connection, song_id)
        .map_err(|error| database_error(error.to_string()))?
    {
        cache::lyrics::upsert_lyrics_cache_entry(remote_connection, &entry)
            .map_err(|error| database_error(error.to_string()))?;
    } else {
        remote_connection
            .execute("DELETE FROM lyrics WHERE song_hash = ?1", [song_id])
            .map_err(|error| database_error(error.to_string()))?;
    }
    Ok(())
}

fn delete_remote_stem_cache_if_present(
    remote_connection: &rusqlite::Connection,
    remote_root: &LibraryRoot,
    song_id: &str,
) -> CommandResult<()> {
    if cache::stems::get_cached_stem_entry(remote_connection, song_id)
        .map_err(|error| database_error(error.to_string()))?
        .is_some()
    {
        cache::stems::delete_stem_cache_entry(remote_connection, remote_root, song_id)
            .map_err(|error| database_error(error.to_string()))?;
    }
    Ok(())
}

fn delete_remote_song_from_mirror(
    app_data_dir: &Path,
    remote_library: &RegisteredLibrary,
    remote_root: &LibraryRoot,
    remote_connection: &rusqlite::Connection,
    song: &Song,
) -> CommandResult<()> {
    if let Some(file_path) = song.file_path.as_deref() {
        remote_delete_relative_path(app_data_dir, remote_library, file_path)?;
    }
    if let Some(cdg_path) = song.cdg_path.as_deref() {
        remote_delete_relative_path(app_data_dir, remote_library, cdg_path)?;
    }
    if song.is_remote_stems()
        || cache::stems::get_cached_stem_entry(remote_connection, &song.hash)
            .map_err(|error| database_error(error.to_string()))?
            .is_some()
    {
        remote_delete_relative_path(app_data_dir, remote_library, &format!("stems/{}", song.hash))?;
    }
    crate::commands::import::delete::delete_song_from_library(remote_connection, remote_root, &song.hash)
        .map_err(|error| library_error(error.to_string()))?;
    Ok(())
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

pub(crate) fn maybe_publish_song_to_bound_remote<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: &str,
) -> CommandResult<()> {
    let config = load_app_config(&state.app_data_dir)?;
    if resolve_active_remote(&config).is_none() {
        return Ok(());
    }

    let local_root = state.library_root()?;
    let local_connection = cache::open_database(&local_root.database_path())
        .map_err(|error| database_error(error.to_string()))?;
    let Some(song) = cache::get_song_by_hash(&local_connection, song_id)
        .map_err(|error| database_error(error.to_string()))?
    else {
        return Ok(());
    };

    if song_ready_for_remote_publish(&local_connection, &local_root, &song)? {
        let _ = publish_song_internal(state, app_handle, song_id)?;
    }

    Ok(())
}

pub(crate) fn maybe_publish_songs_to_bound_remote<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_ids: &[String],
) -> CommandResult<()> {
    for song_id in song_ids {
        maybe_publish_song_to_bound_remote(state, app_handle, song_id)?;
    }
    Ok(())
}

pub(crate) fn sync_bound_remote_for_active_local_library<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
) -> CommandResult<()> {
    let config = load_app_config(&state.app_data_dir)?;
    let Some(remote_library) = resolve_active_remote(&config) else {
        return Ok(());
    };
    let remote_library = prepare_remote_database_for_mutation(&state.app_data_dir, &remote_library)?;

    let local_root = state.library_root()?;
    let local_connection = cache::open_database(&local_root.database_path())
        .map_err(|error| database_error(error.to_string()))?;
    let remote_root = load_remote_root(&state.app_data_dir, &remote_library)?;
    let remote_connection = cache::open_database(&remote_root.database_path())
        .map_err(|error| database_error(error.to_string()))?;
    let local_songs =
        cache::list_songs(&local_connection).map_err(|error| database_error(error.to_string()))?;
    let remote_songs =
        cache::list_songs(&remote_connection).map_err(|error| database_error(error.to_string()))?;

    let mut desired_kinds = HashMap::new();
    for song in &local_songs {
        if let Some(kind) = desired_remote_audio_source_kind(&local_connection, &local_root, song)? {
            desired_kinds.insert(song.hash.clone(), kind);
        }
    }

    for remote_song in &remote_songs {
        match desired_kinds.get(&remote_song.hash) {
            Some(kind) if remote_song.audio_source_kind == *kind => {}
            Some(_) | None => {
                delete_remote_song_from_mirror(
                    &state.app_data_dir,
                    &remote_library,
                    &remote_root,
                    &remote_connection,
                    remote_song,
                )?;
            }
        }
    }

    let desired_song_ids: Vec<String> = local_songs
        .into_iter()
        .filter_map(|song| desired_kinds.contains_key(&song.hash).then_some(song.hash))
        .collect();
    maybe_publish_songs_to_bound_remote(state, app_handle, &desired_song_ids)?;
    let remote_library = load_registered_remote_library(&state.app_data_dir, remote_library.id())?;
    upload_remote_database(&state.app_data_dir, &remote_library)?;
    Ok(())
}

pub(crate) fn mirror_local_library_to_remote<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    local_library_id: &str,
    remote_library_id: &str,
) -> CommandResult<()> {
    let mut config = load_app_config(&state.app_data_dir)?;
    let Some(local_library) = config
        .libraries
        .iter()
        .find(|entry| entry.id() == local_library_id)
    else {
        return Err(library_error(format!(
            "local library {local_library_id} was not found"
        )));
    };
    if !matches!(local_library, RegisteredLibrary::Local { .. }) {
        return Err(library_error(
            "the source library must be a local library".to_owned(),
        ));
    }

    let Some(remote_library) = config
        .libraries
        .iter()
        .find(|entry| entry.id() == remote_library_id)
    else {
        return Err(library_error(format!(
            "remote repository {remote_library_id} was not found"
        )));
    };
    if !matches!(remote_library, RegisteredLibrary::Remote { .. }) {
        return Err(library_error(
            "the target library must be a remote repository".to_owned(),
        ));
    }

    let original_active_library_id = config.active_library_id.clone();
    config.active_library_id = Some(remote_library_id.to_owned());
    persist_app_config(&state.app_data_dir, &config)?;

    let sync_result = sync_bound_remote_for_active_local_library(state, app_handle);

    let mut restore_config = load_app_config(&state.app_data_dir)?;
    restore_config.active_library_id = original_active_library_id;
    persist_app_config(&state.app_data_dir, &restore_config)?;

    sync_result
}

fn publish_song_internal<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: &str,
) -> CommandResult<UploadStatusSnapshot> {
    let config = load_app_config(&state.app_data_dir)?;
    let remote_library = resolve_active_remote(&config)
        .ok_or_else(|| library_error("no bound remote repository is available for publishing"))?;
    let remote_library = prepare_remote_database_for_mutation(&state.app_data_dir, &remote_library)?;

    let local_root = state.library_root()?;
    let remote_library_id = remote_library.id().to_owned();
    let remote_root = load_remote_root(&state.app_data_dir, &remote_library)?;

    // When the active library IS the remote repository (user is directly working
    // in a remote repository), local_root and remote_root point to the same
    // directory.  In that case the "copy to remote" step must be skipped —
    // `copy_directory_recursive` would delete the source before reading it,
    // destroying stems and media files.  The cloud upload reads from the
    // working copy via `RegisteredLibrary::working_copy_root()`, so it works
    // correctly regardless.
    let same_root = local_root.root() == remote_root.root();

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
                    "song {song_id} must have cached stems before publishing to a remote repository"
                ))
            })?;
        if !same_root {
            let source_stems_dir = local_root.resolve(&format!("stems/{song_id}"));
            let destination_stems_dir = remote_root.resolve(&format!("stems/{song_id}"));
            copy_directory_recursive(&source_stems_dir, &destination_stems_dir)?;
        }
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
                    library_error("remote repository is missing a remote locator".to_owned())
                })?;
                google_drive_upload_directory_to_remote(
                    &state.app_data_dir,
                    &remote_library,
                    &remote_secret,
                    &format!("stems/{song_id}"),
                    root_folder_id,
                )?;
            }
            Some(RemoteLibraryProvider::Dropbox) => {
                let remote_secret = load_dropbox_secret(&state.app_data_dir, &remote_library)?;
                let root_path = remote_library.remote_root_locator().ok_or_else(|| {
                    library_error("remote repository is missing a remote locator".to_owned())
                })?;
                dropbox_upload_directory_to_remote(
                    &state.app_data_dir,
                    &remote_library,
                    &remote_secret,
                    &format!("stems/{song_id}"),
                    root_path,
                )?;
            }
            _ => {
                return Err(library_error(
                    "the bound remote provider is not supported for publishing".to_owned(),
                ));
            }
        }
        sync_song_lyrics_to_remote(&local_connection, &remote_connection, song_id)?;
        Ok::<_, CommandError>(())
    } else {
        if let Some(file_path) = song.file_path.as_deref() {
            if !same_root {
                copy_remote_song_assets(&local_root, &remote_root, file_path, file_path)?;
            }
            match remote_library.provider() {
                Some(RemoteLibraryProvider::WebDav) => {
                    let remote_secret = load_webdav_secret(&state.app_data_dir, &remote_library)?;
                    upload_relative_file_to_remote(&remote_library, &remote_secret, file_path)?;
                }
                Some(RemoteLibraryProvider::GoogleDrive) => {
                    let remote_secret = load_google_drive_secret(&state.app_data_dir, &remote_library)?;
                    let root_folder_id = remote_library.remote_root_locator().ok_or_else(|| {
                        library_error("remote repository is missing a remote locator".to_owned())
                    })?;
                    google_drive_upload_relative_file_to_remote(
                        &state.app_data_dir,
                        &remote_library,
                        &remote_secret,
                        file_path,
                        root_folder_id,
                    )?;
                }
                Some(RemoteLibraryProvider::Dropbox) => {
                    let remote_secret = load_dropbox_secret(&state.app_data_dir, &remote_library)?;
                    let root_path = remote_library.remote_root_locator().ok_or_else(|| {
                        library_error("remote repository is missing a remote locator".to_owned())
                    })?;
                    dropbox_upload_relative_file_to_remote(
                        &state.app_data_dir,
                        &remote_library,
                        &remote_secret,
                        file_path,
                        root_path,
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
            if !same_root {
                copy_remote_song_assets(&local_root, &remote_root, cdg_path, cdg_path)?;
            }
            match remote_library.provider() {
                Some(RemoteLibraryProvider::WebDav) => {
                    let remote_secret = load_webdav_secret(&state.app_data_dir, &remote_library)?;
                    upload_relative_file_to_remote(&remote_library, &remote_secret, cdg_path)?;
                }
                Some(RemoteLibraryProvider::GoogleDrive) => {
                    let remote_secret = load_google_drive_secret(&state.app_data_dir, &remote_library)?;
                    let root_folder_id = remote_library.remote_root_locator().ok_or_else(|| {
                        library_error("remote repository is missing a remote locator".to_owned())
                    })?;
                    google_drive_upload_relative_file_to_remote(
                        &state.app_data_dir,
                        &remote_library,
                        &remote_secret,
                        cdg_path,
                        root_folder_id,
                    )?;
                }
                Some(RemoteLibraryProvider::Dropbox) => {
                    let remote_secret = load_dropbox_secret(&state.app_data_dir, &remote_library)?;
                    let root_path = remote_library.remote_root_locator().ok_or_else(|| {
                        library_error("remote repository is missing a remote locator".to_owned())
                    })?;
                    dropbox_upload_relative_file_to_remote(
                        &state.app_data_dir,
                        &remote_library,
                        &remote_secret,
                        cdg_path,
                        root_path,
                    )?;
                }
                _ => {
                    return Err(library_error(
                        "the bound remote provider is not supported for publishing".to_owned(),
                    ));
                }
            }
        }

        delete_remote_stem_cache_if_present(&remote_connection, &remote_root, song_id)?;

        update_remote_song(&remote_connection, song.clone(), "original_remote")?;
        sync_song_lyrics_to_remote(&local_connection, &remote_connection, song_id)?;
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

pub(crate) fn sync_active_remote_library(state: &AppState) -> CommandResult<()> {
    let config = load_app_config(&state.app_data_dir)?;
    let Some(active_library) = config.active_library() else {
        return Err(library_error("no library is currently active"));
    };

    if matches!(active_library, RegisteredLibrary::Remote { .. }) {
        let _ = sync_remote_database_from_provider(&state.app_data_dir, active_library)?;
    }

    Ok(())
}

pub(crate) fn publish_song_to_remote<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: String,
) -> CommandResult<UploadStatusSnapshot> {
    publish_song_internal(state, app_handle, &song_id)
}

pub(crate) fn publish_songs_to_remote<R: tauri::Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_ids: Vec<String>,
) -> CommandResult<Vec<UploadStatusSnapshot>> {
    let mut snapshots = Vec::with_capacity(song_ids.len());
    for song_id in song_ids {
        snapshots.push(publish_song_internal(state, app_handle, &song_id)?);
    }
    Ok(snapshots)
}

pub(crate) fn get_all_upload_statuses(state: &AppState) -> CommandResult<Vec<UploadStatusSnapshot>> {
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
    fn stored_revision_is_stale_when_provider_revision_changed() {
        assert!(!remote_database_revision_is_stale(None, None));
        assert!(!remote_database_revision_is_stale(Some("rev-1"), None));
        assert!(!remote_database_revision_is_stale(Some("rev-1"), Some("rev-1")));
        assert!(remote_database_revision_is_stale(None, Some("rev-1")));
        assert!(remote_database_revision_is_stale(Some("rev-1"), Some("rev-2")));
    }

    #[test]
    fn database_conflict_error_points_to_settings_recovery_actions() {
        let error = remote_database_conflict_error(Some("rev-2"));

        assert!(error.retryable);
        assert!(error.message.contains("Refresh remote repository"));
        assert!(error.message.contains("Reauthorize remote repository"));
    }
}
