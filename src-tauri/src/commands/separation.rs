use crate::{
    cache,
    commands::error::{separation_error, state_lock_error, CommandError, CommandResult},
    config::{self, StemMode},
    separator, AppState,
};
use serde::Serialize;
use std::{
    collections::HashMap,
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

#[tauri::command]
pub fn separate(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<SeparationStatusSnapshot> {
    crate::commands::bootstrap::ensure_model_ready(&state.model_bootstrap_status)?;

    let initial_status = {
        let mut statuses = state
            .separation_statuses
            .lock()
            .map_err(|_| state_lock_error("separation status lock was poisoned"))?;

        if let Some(existing) = statuses.get(&song_id) {
            if existing.state == SeparationState::Running {
                return Ok(existing.clone());
            }
        }

        let status = running_status(&song_id, 0);
        statuses.insert(song_id.clone(), status.clone());
        status
    };

    let library_root = state.library_root()?;
    let model_path = state.model_path.clone();
    let separation_statuses = Arc::clone(&state.separation_statuses);
    let worker_song_id = song_id.clone();
    let worker_app_handle = app_handle.clone();
    let stem_mode = config::load_config(&state.app_data_dir)
        .ok()
        .flatten()
        .map(|c| c.effective_stem_mode())
        .unwrap_or_default();

    tauri::async_runtime::spawn(async move {
        let worker_library_root = library_root.clone();
        let worker_model_path = model_path.clone();
        let worker_statuses = Arc::clone(&separation_statuses);
        let progress_song_id = worker_song_id.clone();
        let progress_app_handle = worker_app_handle.clone();

        let result = tauri::async_runtime::spawn_blocking(move || {
            let connection = cache::open_database(&worker_library_root.database_path())?;
            separator::job::separate_song_into_cache(
                &connection,
                &worker_library_root,
                &worker_model_path,
                &worker_song_id,
                stem_mode,
                |percent| {
                    let snapshot = running_status(&progress_song_id, percent);
                    if let Ok(mut statuses) = worker_statuses.lock() {
                        statuses.insert(progress_song_id.clone(), snapshot);
                    }
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

        match result {
            Ok(Ok(artifacts)) => {
                let completed = completed_status(
                    &song_id,
                    artifacts.vocals_path,
                    artifacts.accomp_path,
                    artifacts.cache_hit,
                    artifacts.drums_path,
                    artifacts.bass_path,
                    artifacts.other_path,
                );
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), completed);
                }
                let _ = app_handle.emit(
                    SEPARATION_COMPLETE_EVENT,
                    SeparationCompleteEvent {
                        song_id: song_id.clone(),
                    },
                );
            }
            Ok(Err(error)) => {
                let command_error = separation_error(error.to_string());
                let failed = failed_status(&song_id, command_error.clone());
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), failed);
                }
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent {
                        song_id: song_id.clone(),
                        error: command_error,
                    },
                );
            }
            Err(error) => {
                let command_error = separation_error(error.to_string());
                let failed = failed_status(&song_id, command_error.clone());
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), failed);
                }
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent {
                        song_id: song_id.clone(),
                        error: command_error,
                    },
                );
            }
        }
    });

    Ok(initial_status)
}

#[tauri::command]
pub fn upgrade_to_four_stem(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<SeparationStatusSnapshot> {
    crate::commands::bootstrap::ensure_model_ready(&state.model_bootstrap_status)?;

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

    let initial_status = {
        let mut statuses = state
            .separation_statuses
            .lock()
            .map_err(|_| state_lock_error("separation status lock was poisoned"))?;

        if let Some(existing) = statuses.get(&song_id) {
            if existing.state == SeparationState::Running {
                return Ok(existing.clone());
            }
        }

        let status = running_status(&song_id, 0);
        statuses.insert(song_id.clone(), status.clone());
        status
    };

    let library_root = state.library_root()?;
    let model_path = state.model_path.clone();
    let separation_statuses = Arc::clone(&state.separation_statuses);
    let worker_song_id = song_id.clone();
    let worker_app_handle = app_handle.clone();

    tauri::async_runtime::spawn(async move {
        let worker_library_root = library_root.clone();
        let worker_model_path = model_path.clone();
        let worker_statuses = Arc::clone(&separation_statuses);
        let progress_song_id = worker_song_id.clone();
        let progress_app_handle = worker_app_handle.clone();

        let result = tauri::async_runtime::spawn_blocking(move || {
            let connection = cache::open_database(&worker_library_root.database_path())?;
            separator::job::separate_song_into_cache(
                &connection,
                &worker_library_root,
                &worker_model_path,
                &worker_song_id,
                StemMode::FourStem,
                |percent| {
                    let snapshot = running_status(&progress_song_id, percent);
                    if let Ok(mut statuses) = worker_statuses.lock() {
                        statuses.insert(progress_song_id.clone(), snapshot);
                    }
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

        match result {
            Ok(Ok(artifacts)) => {
                let completed = completed_status(
                    &song_id,
                    artifacts.vocals_path,
                    artifacts.accomp_path,
                    artifacts.cache_hit,
                    artifacts.drums_path,
                    artifacts.bass_path,
                    artifacts.other_path,
                );
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), completed);
                }
                let _ = app_handle.emit(
                    SEPARATION_COMPLETE_EVENT,
                    SeparationCompleteEvent {
                        song_id: song_id.clone(),
                    },
                );
            }
            Ok(Err(error)) => {
                let command_error = separation_error(error.to_string());
                let failed = failed_status(&song_id, command_error.clone());
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), failed);
                }
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent {
                        song_id: song_id.clone(),
                        error: command_error,
                    },
                );
            }
            Err(error) => {
                let command_error = separation_error(error.to_string());
                let failed = failed_status(&song_id, command_error.clone());
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), failed);
                }
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent {
                        song_id: song_id.clone(),
                        error: command_error,
                    },
                );
            }
        }
    });

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

    // Clear existing cache entry and stem files.
    {
        let library_root = state.library_root()?;
        let connection = cache::open_database(&library_root.database_path())
            .map_err(|e| separation_error(e.to_string()))?;
        let _ = cache::stems::delete_stem_cache_entry(&connection, &library_root, &song_id);
    }

    // Remove in-memory status so the song appears idle before re-separation starts.
    {
        let mut statuses = state
            .separation_statuses
            .lock()
            .map_err(|_| state_lock_error("separation status lock was poisoned"))?;
        statuses.remove(&song_id);
    }

    let initial_status = {
        let mut statuses = state
            .separation_statuses
            .lock()
            .map_err(|_| state_lock_error("separation status lock was poisoned"))?;

        let status = running_status(&song_id, 0);
        statuses.insert(song_id.clone(), status.clone());
        status
    };

    let library_root = state.library_root()?;
    let model_path = state.model_path.clone();
    let separation_statuses = Arc::clone(&state.separation_statuses);
    let worker_song_id = song_id.clone();
    let worker_app_handle = app_handle.clone();

    tauri::async_runtime::spawn(async move {
        let worker_library_root = library_root.clone();
        let worker_model_path = model_path.clone();
        let worker_statuses = Arc::clone(&separation_statuses);
        let progress_song_id = worker_song_id.clone();
        let progress_app_handle = worker_app_handle.clone();

        let result = tauri::async_runtime::spawn_blocking(move || {
            let connection = cache::open_database(&worker_library_root.database_path())?;
            separator::job::separate_song_into_cache(
                &connection,
                &worker_library_root,
                &worker_model_path,
                &worker_song_id,
                stem_mode,
                |percent| {
                    let snapshot = running_status(&progress_song_id, percent);
                    if let Ok(mut statuses) = worker_statuses.lock() {
                        statuses.insert(progress_song_id.clone(), snapshot);
                    }
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

        match result {
            Ok(Ok(artifacts)) => {
                let completed = completed_status(
                    &song_id,
                    artifacts.vocals_path,
                    artifacts.accomp_path,
                    artifacts.cache_hit,
                    artifacts.drums_path,
                    artifacts.bass_path,
                    artifacts.other_path,
                );
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), completed);
                }
                let _ = app_handle.emit(
                    SEPARATION_COMPLETE_EVENT,
                    SeparationCompleteEvent {
                        song_id: song_id.clone(),
                    },
                );
            }
            Ok(Err(error)) => {
                let command_error = separation_error(error.to_string());
                let failed = failed_status(&song_id, command_error.clone());
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), failed);
                }
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent {
                        song_id: song_id.clone(),
                        error: command_error,
                    },
                );
            }
            Err(error) => {
                let command_error = separation_error(error.to_string());
                let failed = failed_status(&song_id, command_error.clone());
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), failed);
                }
                let _ = app_handle.emit(
                    SEPARATION_ERROR_EVENT,
                    SeparationErrorEvent {
                        song_id: song_id.clone(),
                        error: command_error,
                    },
                );
            }
        }
    });

    Ok(initial_status)
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
            let status = completed_status(
                &entry.song_hash,
                &entry.vocals_path,
                &entry.accomp_path,
                true,
                entry.drums_path.clone(),
                entry.bass_path.clone(),
                entry.other_path.clone(),
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
        error: Some(error),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn status_lookup_defaults_to_idle_when_song_has_not_started_separation() {
        let statuses = Arc::new(Mutex::new(HashMap::new()));

        let status = get_separation_status_from_map(&statuses, "missing-song")
            .expect("idle lookup should succeed");

        assert_eq!(status, idle_status("missing-song"));
    }
}
