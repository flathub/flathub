use crate::{
    cache,
    commands::error::{separation_error, state_lock_error, CommandError, CommandResult},
    config::{self, ExecutionProviderPreference, StemMode},
    separator::{self, model::LoadedModel, model_cache::ModelCache},
    AppState,
};
use serde::Serialize;
use std::{
    collections::HashMap,
    path::PathBuf,
    sync::{Arc, Mutex},
};
use tauri::{AppHandle, Emitter, State};

pub const SEPARATION_PROGRESS_EVENT: &str = "separation-progress";
pub const SEPARATION_COMPLETE_EVENT: &str = "separation-complete";
pub const SEPARATION_ERROR_EVENT: &str = "separation-error";

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum SeparationState {
    Idle,
    Running,
    Completed,
    Failed,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct SeparationStatusSnapshot {
    pub song_id: String,
    pub state: SeparationState,
    pub percent: u8,
    pub cache_hit: bool,
    pub vocals_path: Option<String>,
    pub accomp_path: Option<String>,
    pub drums_path: Option<String>,
    pub bass_path: Option<String>,
    pub other_path: Option<String>,
    pub model_variant: Option<String>,
    pub error: Option<CommandError>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct SeparationProgressEvent {
    pub song_id: String,
    pub percent: u8,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct SeparationCompleteEvent {
    pub song_id: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct SeparationErrorEvent {
    pub song_id: String,
    pub error: CommandError,
}

struct SeparationExecutionContext {
    library_root: crate::library_root::LibraryRoot,
    model_path: PathBuf,
    model_variant: String,
    ep_preference: ExecutionProviderPreference,
    statuses: Arc<Mutex<HashMap<String, SeparationStatusSnapshot>>>,
    model_cache: Arc<Mutex<ModelCache<LoadedModel>>>,
}

#[tauri::command]
pub fn separate(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<SeparationStatusSnapshot> {
    crate::commands::bootstrap::ensure_model_ready(&state.model_bootstrap_status)?;
    ensure_song_can_be_separated(&state, &song_id)?;

    let initial_status = reserve_running_status(&state.separation_statuses, &song_id, true)?;
    let app_config = config::load_config(&state.app_data_dir).ok().flatten();
    let stem_mode = app_config
        .as_ref()
        .map(|c| c.effective_stem_mode())
        .unwrap_or_default();
    let execution_context = build_execution_context(&state)?;

    spawn_separation_job(app_handle, execution_context, song_id.clone(), stem_mode);

    Ok(initial_status)
}

#[tauri::command]
pub fn upgrade_to_four_stem(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<SeparationStatusSnapshot> {
    crate::commands::bootstrap::ensure_model_ready(&state.model_bootstrap_status)?;
    ensure_song_can_be_separated(&state, &song_id)?;

    // Check if song already has 4-stem separation cached.
    {
        let library_root = state.library_root()?;
        let connection = cache::open_database(&library_root.database_path())
            .map_err(|e| separation_error(e.to_string()))?;
        if let Ok(Some(entry)) = cache::stems::get_cached_stem_entry(&connection, &song_id) {
            if entry.has_individual_stems() {
                return Ok(completed_status(
                    &song_id,
                    entry.vocals_path,
                    entry.accomp_path,
                    true,
                    entry.drums_path,
                    entry.bass_path,
                    entry.other_path,
                ));
            }
        }
    }

    let initial_status = reserve_running_status(&state.separation_statuses, &song_id, true)?;
    let execution_context = build_execution_context(&state)?;

    spawn_separation_job(
        app_handle,
        execution_context,
        song_id.clone(),
        StemMode::FourStem,
    );

    Ok(initial_status)
}

#[tauri::command]
pub fn re_separate(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
    stem_mode: StemMode,
) -> CommandResult<SeparationStatusSnapshot> {
    crate::commands::bootstrap::ensure_model_ready(&state.model_bootstrap_status)?;
    ensure_song_can_be_separated(&state, &song_id)?;

    // Clear existing cache entry and stem files before relaunching separation.
    {
        let library_root = state.library_root()?;
        let connection = cache::open_database(&library_root.database_path())
            .map_err(|e| separation_error(e.to_string()))?;
        let _ = cache::stems::delete_stem_cache_entry(&connection, &library_root, &song_id);
    }

    {
        let mut statuses = state
            .separation_statuses
            .lock()
            .map_err(|_| state_lock_error("separation status lock was poisoned"))?;
        statuses.remove(&song_id);
    }

    let initial_status = reserve_running_status(&state.separation_statuses, &song_id, false)?;
    let execution_context = build_execution_context(&state)?;

    spawn_separation_job(app_handle, execution_context, song_id.clone(), stem_mode);

    Ok(initial_status)
}

// Keep the command handlers thin and preserve the event/status contract in one
// place. Separation orchestration is concurrency-sensitive, so duplicated
// branches here are easy to drift and regress independently.
fn spawn_separation_job(
    app_handle: AppHandle,
    execution_context: SeparationExecutionContext,
    song_id: String,
    stem_mode: StemMode,
) {
    let SeparationExecutionContext {
        library_root,
        model_path,
        model_variant,
        ep_preference,
        statuses,
        model_cache,
    } = execution_context;
    let progress_song_id = song_id.clone();
    let progress_app_handle = app_handle.clone();
    let progress_statuses = Arc::clone(&statuses);

    tauri::async_runtime::spawn(async move {
        let worker_library_root = library_root.clone();
        let worker_model_path = model_path.clone();
        let worker_song_id = song_id.clone();

        let result = tauri::async_runtime::spawn_blocking(move || {
            let connection = cache::open_database(&worker_library_root.database_path())?;
            separator::job::separate_song_into_cache(
                &connection,
                &worker_library_root,
                &model_cache,
                &worker_model_path,
                &worker_song_id,
                stem_mode,
                &model_variant,
                ep_preference,
                |percent| {
                    let snapshot = running_status(&progress_song_id, percent);
                    store_status(&progress_statuses, &progress_song_id, snapshot);
                    let _ = progress_app_handle.emit(
                        SEPARATION_PROGRESS_EVENT,
                        SeparationProgressEvent {
                            song_id: progress_song_id.clone(),
                            percent,
                        },
                    );
                },
            )
        })
        .await;

        let final_status = match result {
            Ok(Ok(artifacts)) => status_from_job_result(&song_id, Ok(artifacts)),
            Ok(Err(error)) => {
                status_from_job_result(&song_id, Err(separation_error(error.to_string())))
            }
            Err(error) => {
                status_from_job_result(&song_id, Err(separation_error(error.to_string())))
            }
        };

        emit_terminal_status(&app_handle, &statuses, final_status);
    });
}

fn emit_terminal_status(
    app_handle: &AppHandle,
    statuses: &Arc<Mutex<HashMap<String, SeparationStatusSnapshot>>>,
    status: SeparationStatusSnapshot,
) {
    let song_id = status.song_id.clone();
    let error = status.error.clone();
    let state = status.state.clone();
    store_status(statuses, &song_id, status);

    match state {
        SeparationState::Completed => {
            let _ = app_handle.emit(
                SEPARATION_COMPLETE_EVENT,
                SeparationCompleteEvent { song_id },
            );
        }
        SeparationState::Failed => {
            if let Some(error) = error {
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent { song_id, error },
                );
            }
        }
        SeparationState::Idle | SeparationState::Running => {}
    }
}

fn build_execution_context(
    state: &State<'_, AppState>,
) -> CommandResult<SeparationExecutionContext> {
    let app_config = config::load_config(&state.app_data_dir).ok().flatten();
    let model_variant = app_config
        .as_ref()
        .map(|c| c.effective_model_variant())
        .unwrap_or_default()
        .as_str()
        .to_owned();
    let ep_preference = app_config
        .as_ref()
        .map(|c| c.effective_execution_provider())
        .unwrap_or_default();

    Ok(SeparationExecutionContext {
        library_root: state.library_root()?,
        model_path: state.resolve_model_path()?,
        model_variant,
        ep_preference,
        statuses: Arc::clone(&state.separation_statuses),
        model_cache: Arc::clone(&state.separator_model_cache),
    })
}

fn reserve_running_status(
    statuses: &Arc<Mutex<HashMap<String, SeparationStatusSnapshot>>>,
    song_id: &str,
    allow_existing_running: bool,
) -> CommandResult<SeparationStatusSnapshot> {
    let mut statuses = statuses
        .lock()
        .map_err(|_| state_lock_error("separation status lock was poisoned"))?;
    if allow_existing_running {
        if let Some(existing) = statuses.get(song_id) {
            if existing.state == SeparationState::Running {
                return Ok(existing.clone());
            }
        }
    }
    let status = running_status(song_id, 0);
    statuses.insert(song_id.to_owned(), status.clone());
    Ok(status)
}

fn status_from_job_result(
    song_id: &str,
    result: Result<crate::separator::job::SeparationArtifacts, CommandError>,
) -> SeparationStatusSnapshot {
    match result {
        Ok(artifacts) => completed_status(
            song_id,
            artifacts.vocals_path,
            artifacts.accomp_path,
            artifacts.cache_hit,
            artifacts.drums_path,
            artifacts.bass_path,
            artifacts.other_path,
        ),
        Err(error) => failed_status(song_id, error),
    }
}

fn store_status(
    statuses: &Arc<Mutex<HashMap<String, SeparationStatusSnapshot>>>,
    song_id: &str,
    status: SeparationStatusSnapshot,
) {
    if let Ok(mut statuses) = statuses.lock() {
        statuses.insert(song_id.to_owned(), status);
    }
}

#[tauri::command]
pub fn downgrade_single_to_two_stem(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<SeparationStatusSnapshot> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path())
        .map_err(|e| separation_error(e.to_string()))?;

    let (updated_entry, _freed_bytes) =
        cache::stems::downgrade_to_two_stem(&connection, &library_root, &song_id)
            .map_err(|e| separation_error(e.to_string()))?;

    let completed = completed_status(
        &song_id,
        &updated_entry.vocals_path,
        &updated_entry.accomp_path,
        true,
        updated_entry.drums_path,
        updated_entry.bass_path,
        updated_entry.other_path,
    );

    // Update in-memory separation statuses.
    {
        let mut statuses = state
            .separation_statuses
            .lock()
            .map_err(|_| state_lock_error("separation status lock was poisoned"))?;
        statuses.insert(song_id.clone(), completed.clone());
    }

    // Emit completion event so the frontend updates.
    let _ = app_handle.emit(
        SEPARATION_COMPLETE_EVENT,
        SeparationCompleteEvent {
            song_id: song_id.clone(),
        },
    );

    Ok(completed)
}

#[tauri::command]
pub fn get_separation_status(
    state: State<'_, AppState>,
    song_id: String,
) -> CommandResult<SeparationStatusSnapshot> {
    get_separation_status_from_map(&state.separation_statuses, &song_id)
}

/// Returns separation statuses for all songs that have cached stems in the database.
/// Called once at startup to hydrate the frontend store so that previously separated
/// songs show as "completed" instead of "idle".
#[tauri::command]
pub fn get_all_separation_statuses(
    state: State<'_, AppState>,
) -> CommandResult<Vec<SeparationStatusSnapshot>> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path())
        .map_err(|e| crate::commands::error::database_error(e.to_string()))?;

    let entries = cache::stems::list_all_stem_entries(&connection)
        .map_err(|e| crate::commands::error::database_error(e.to_string()))?;

    // Also populate the in-memory separation_statuses map so that
    // subsequent get_separation_status calls return the correct state.
    let mut statuses_lock = state
        .separation_statuses
        .lock()
        .map_err(|_| state_lock_error("separation status lock was poisoned"))?;

    let mut result = Vec::with_capacity(entries.len());
    for entry in entries {
        // Only report entries whose files still exist on disk
        if cache::stems::cache_entry_files_valid(&library_root, &entry) {
            let status = completed_status_with_model(
                &entry.song_hash,
                &entry.vocals_path,
                &entry.accomp_path,
                true,
                entry.drums_path.clone(),
                entry.bass_path.clone(),
                entry.other_path.clone(),
                entry.model_variant.clone(),
            );
            statuses_lock.insert(entry.song_hash.clone(), status.clone());
            result.push(status);
        }
    }

    Ok(result)
}

pub fn get_separation_status_from_map(
    statuses: &Arc<Mutex<HashMap<String, SeparationStatusSnapshot>>>,
    song_id: &str,
) -> CommandResult<SeparationStatusSnapshot> {
    let statuses = statuses
        .lock()
        .map_err(|_| state_lock_error("separation status lock was poisoned"))?;

    Ok(statuses
        .get(song_id)
        .cloned()
        .unwrap_or_else(|| idle_status(song_id)))
}

fn ensure_song_can_be_separated(state: &State<'_, AppState>, song_id: &str) -> CommandResult<()> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path())
        .map_err(|e| separation_error(e.to_string()))?;
    let song = cache::get_song_by_hash(&connection, song_id)
        .map_err(|e| separation_error(e.to_string()))?
        .ok_or_else(|| {
            separation_error(format!(
                "song with hash {song_id} was not found in the library"
            ))
        })?;

    validate_song_can_be_separated(&song, song_id)
}

fn validate_song_can_be_separated(song: &crate::library::Song, song_id: &str) -> CommandResult<()> {
    if song.is_media_g() {
        // Media+G songs already carry karaoke graphics and intentionally skip
        // the stem pipeline, which is designed for plain audio assets only.
        return Err(separation_error(format!(
            "song {song_id} is a Media+G track and cannot be separated"
        )));
    }

    if song.is_instrumental() {
        return Err(separation_error(format!(
            "song {song_id} is marked instrumental and cannot be separated"
        )));
    }

    Ok(())
}

pub fn idle_status(song_id: impl Into<String>) -> SeparationStatusSnapshot {
    SeparationStatusSnapshot {
        song_id: song_id.into(),
        state: SeparationState::Idle,
        percent: 0,
        cache_hit: false,
        vocals_path: None,
        accomp_path: None,
        drums_path: None,
        bass_path: None,
        other_path: None,
        model_variant: None,
        error: None,
    }
}

pub fn running_status(song_id: impl Into<String>, percent: u8) -> SeparationStatusSnapshot {
    SeparationStatusSnapshot {
        song_id: song_id.into(),
        state: SeparationState::Running,
        percent: percent.min(100),
        cache_hit: false,
        vocals_path: None,
        accomp_path: None,
        drums_path: None,
        bass_path: None,
        other_path: None,
        model_variant: None,
        error: None,
    }
}

pub fn completed_status(
    song_id: impl Into<String>,
    vocals_path: impl Into<String>,
    accomp_path: impl Into<String>,
    cache_hit: bool,
    drums_path: Option<String>,
    bass_path: Option<String>,
    other_path: Option<String>,
) -> SeparationStatusSnapshot {
    SeparationStatusSnapshot {
        song_id: song_id.into(),
        state: SeparationState::Completed,
        percent: 100,
        cache_hit,
        vocals_path: Some(vocals_path.into()),
        accomp_path: Some(accomp_path.into()),
        drums_path,
        bass_path,
        other_path,
        model_variant: None,
        error: None,
    }
}

pub fn completed_status_with_model(
    song_id: impl Into<String>,
    vocals_path: impl Into<String>,
    accomp_path: impl Into<String>,
    cache_hit: bool,
    drums_path: Option<String>,
    bass_path: Option<String>,
    other_path: Option<String>,
    model_variant: String,
) -> SeparationStatusSnapshot {
    SeparationStatusSnapshot {
        song_id: song_id.into(),
        state: SeparationState::Completed,
        percent: 100,
        cache_hit,
        vocals_path: Some(vocals_path.into()),
        accomp_path: Some(accomp_path.into()),
        drums_path,
        bass_path,
        other_path,
        model_variant: Some(model_variant),
        error: None,
    }
}

pub fn failed_status(song_id: impl Into<String>, error: CommandError) -> SeparationStatusSnapshot {
    SeparationStatusSnapshot {
        song_id: song_id.into(),
        state: SeparationState::Failed,
        percent: 100,
        cache_hit: false,
        vocals_path: None,
        accomp_path: None,
        drums_path: None,
        bass_path: None,
        other_path: None,
        model_variant: None,
        error: Some(error),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::separator::job::SeparationArtifacts;

    #[test]
    fn status_lookup_defaults_to_idle_when_song_has_not_started_separation() {
        let statuses = Arc::new(Mutex::new(HashMap::new()));

        let status = get_separation_status_from_map(&statuses, "missing-song")
            .expect("idle lookup should succeed");

        assert_eq!(status, idle_status("missing-song"));
    }

    #[test]
    fn reserve_running_status_reuses_existing_running_entry_when_allowed() {
        let statuses = Arc::new(Mutex::new(HashMap::from([(
            "song-1".to_owned(),
            running_status("song-1", 42),
        )])));

        let status = reserve_running_status(&statuses, "song-1", true)
            .expect("running status reservation should succeed");

        assert_eq!(status, running_status("song-1", 42));
        assert_eq!(
            statuses
                .lock()
                .expect("status map lock should succeed")
                .get("song-1")
                .cloned(),
            Some(running_status("song-1", 42))
        );
    }

    #[test]
    fn status_from_job_result_maps_success_to_completed_status() {
        let status = status_from_job_result(
            "song-1",
            Ok(SeparationArtifacts {
                vocals_path: "vocals.ogg".to_owned(),
                accomp_path: "accomp.ogg".to_owned(),
                cache_hit: true,
                drums_path: Some("drums.ogg".to_owned()),
                bass_path: Some("bass.ogg".to_owned()),
                other_path: Some("other.ogg".to_owned()),
            }),
        );

        assert_eq!(
            status,
            completed_status(
                "song-1",
                "vocals.ogg",
                "accomp.ogg",
                true,
                Some("drums.ogg".to_owned()),
                Some("bass.ogg".to_owned()),
                Some("other.ogg".to_owned()),
            )
        );
    }

    #[test]
    fn status_from_job_result_maps_errors_to_failed_status() {
        let error = separation_error("boom".to_owned());

        let status = status_from_job_result("song-1", Err(error.clone()));

        assert_eq!(status, failed_status("song-1", error));
    }

    #[test]
    fn validate_song_can_be_separated_rejects_instrumental_songs() {
        let song = crate::library::Song {
            hash: "song-1".to_owned(),
            file_path: "media/song-1.mp3".to_owned(),
            cdg_path: None,
            media_g_container: None,
            instrumental: true,
            title: Some("Song".to_owned()),
            artist: None,
            album: None,
            duration_ms: 1_000,
            cover_art: None,
            imported_at: 1,
            original_ext: Some("mp3".to_owned()),
        };

        let error = validate_song_can_be_separated(&song, "song-1")
            .expect_err("instrumental songs should be rejected");

        assert!(error.message.contains("marked instrumental"));
    }
}
