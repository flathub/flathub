use crate::{
    cache,
    commands::error::{database_error, separation_error, CommandResult},
    commands::separation::{
        completed_status, failed_status, running_status, SeparationCompleteEvent,
        SeparationErrorEvent, SeparationProgressEvent, SEPARATION_COMPLETE_EVENT,
        SEPARATION_ERROR_EVENT, SEPARATION_PROGRESS_EVENT,
    },
    config::{self, StemMode},
    separator::{self, model::LoadedModel, model_cache::ModelCache},
    AppState,
};
use serde::Serialize;
use std::sync::{atomic::Ordering, Arc, Mutex};
use tauri::{AppHandle, Emitter, State};

pub const BATCH_SEPARATION_PROGRESS_EVENT: &str = "batch-separation-progress";
pub const BATCH_SEPARATION_COMPLETE_EVENT: &str = "batch-separation-complete";
pub const BATCH_SEPARATION_CANCELLED_EVENT: &str = "batch-separation-cancelled";

#[derive(Debug, Clone, Serialize)]
pub struct BatchSeparationProgress {
    pub total: usize,
    pub completed: usize,
    pub skipped: usize,
    pub failed: usize,
    pub current_song_id: Option<String>,
    pub current_percent: u8,
}

/// Batch separate songs. If `song_ids` is empty, separate all songs in the library.
/// Songs are processed sequentially (ONNX Runtime is memory-heavy).
#[tauri::command]
pub fn batch_separate(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_ids: Vec<String>,
) -> CommandResult<()> {
    crate::commands::bootstrap::ensure_model_ready(&state.model_bootstrap_status)?;

    // Prevent concurrent batch operations.
    if state.batch_running.load(Ordering::Relaxed) {
        return Err(separation_error(
            "A batch separation is already running".to_owned(),
        ));
    }

    let library_root = state.library_root()?;
    let model_path = state.resolve_model_path()?;
    let separation_statuses = Arc::clone(&state.separation_statuses);
    let model_cache: Arc<Mutex<ModelCache<LoadedModel>>> = Arc::clone(&state.separator_model_cache);
    let batch_running = Arc::clone(&state.batch_running);
    let batch_cancel = Arc::clone(&state.batch_cancel);

    let app_config = config::load_config(&state.app_data_dir).ok().flatten();
    let stem_mode = app_config
        .as_ref()
        .map(|c| c.effective_stem_mode())
        .unwrap_or_default();
    let model_variant_str = app_config
        .as_ref()
        .map(|c| c.effective_model_variant())
        .unwrap_or_default()
        .as_str()
        .to_owned();

    // Resolve the list of song hashes to process.
    let hashes: Vec<String> = if song_ids.is_empty() {
        let connection = cache::open_database(&library_root.database_path())
            .map_err(|e| database_error(e.to_string()))?;
        let songs = cache::list_songs(&connection).map_err(|e| database_error(e.to_string()))?;
        songs
            .into_iter()
            .filter(|song| song.is_separable())
            .map(|s| s.hash)
            .collect()
    } else {
        let connection = cache::open_database(&library_root.database_path())
            .map_err(|e| database_error(e.to_string()))?;
        song_ids
            .into_iter()
            .filter(|song_id| {
                cache::get_song_by_hash(&connection, song_id)
                    .ok()
                    .flatten()
                    .map(|song| song.is_separable())
                    .unwrap_or(false)
            })
            .collect()
    };

    // Filter out already-separated songs.
    let connection = cache::open_database(&library_root.database_path())
        .map_err(|e| database_error(e.to_string()))?;
    let mut to_separate = Vec::new();
    let mut skipped: usize = 0;
    for hash in &hashes {
        if let Ok(Some(entry)) = cache::stems::get_cached_stem_entry(&connection, hash) {
            let already_done = match stem_mode {
                StemMode::TwoStem => true, // any cached entry is sufficient
                StemMode::FourStem => entry.has_individual_stems(),
            };
            if already_done && cache::stems::cache_entry_files_valid(&library_root, &entry) {
                skipped += 1;
                continue;
            }
        }
        to_separate.push(hash.clone());
    }
    drop(connection);

    let total = to_separate.len();

    // Mark batch as running.
    batch_running.store(true, Ordering::Relaxed);
    batch_cancel.store(false, Ordering::Relaxed);

    // Emit initial progress.
    let _ = app_handle.emit(
        BATCH_SEPARATION_PROGRESS_EVENT,
        BatchSeparationProgress {
            total,
            completed: 0,
            skipped,
            failed: 0,
            current_song_id: None,
            current_percent: 0,
        },
    );

    tauri::async_runtime::spawn(async move {
        let mut completed: usize = 0;
        let mut failed_count: usize = 0;

        for song_id in &to_separate {
            // Check cancellation.
            if batch_cancel.load(Ordering::Relaxed) {
                let _ = app_handle.emit(
                    BATCH_SEPARATION_CANCELLED_EVENT,
                    BatchSeparationProgress {
                        total,
                        completed,
                        skipped,
                        failed: failed_count,
                        current_song_id: None,
                        current_percent: 0,
                    },
                );
                batch_running.store(false, Ordering::Relaxed);
                return;
            }

            // Mark song as running.
            {
                if let Ok(mut statuses) = separation_statuses.lock() {
                    statuses.insert(song_id.clone(), running_status(song_id, 0));
                }
            }

            // Emit batch progress with current song.
            let _ = app_handle.emit(
                BATCH_SEPARATION_PROGRESS_EVENT,
                BatchSeparationProgress {
                    total,
                    completed,
                    skipped,
                    failed: failed_count,
                    current_song_id: Some(song_id.clone()),
                    current_percent: 0,
                },
            );

            let worker_library_root = library_root.clone();
            let worker_model_path = model_path.clone();
            let worker_song_id = song_id.clone();
            let worker_statuses = Arc::clone(&separation_statuses);
            let worker_model_cache = Arc::clone(&model_cache);
            let progress_song_id = song_id.clone();
            let progress_app_handle = app_handle.clone();
            let batch_progress_app_handle = app_handle.clone();
            let batch_total = total;
            let batch_completed = completed;
            let batch_skipped = skipped;
            let batch_failed = failed_count;

            let worker_model_variant = model_variant_str.clone();
            let result = tauri::async_runtime::spawn_blocking(move || {
                let connection = cache::open_database(&worker_library_root.database_path())?;
                separator::job::separate_song_into_cache(
                    &connection,
                    &worker_library_root,
                    &worker_model_cache,
                    &worker_model_path,
                    &worker_song_id,
                    stem_mode,
                    &worker_model_variant,
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
                        // Also emit batch progress update with per-song percent.
                        let _ = batch_progress_app_handle.emit(
                            BATCH_SEPARATION_PROGRESS_EVENT,
                            BatchSeparationProgress {
                                total: batch_total,
                                completed: batch_completed,
                                skipped: batch_skipped,
                                failed: batch_failed,
                                current_song_id: Some(progress_song_id.clone()),
                                current_percent: percent,
                            },
                        );
                    },
                )
            })
            .await;

            match result {
                Ok(Ok(artifacts)) => {
                    let status = completed_status(
                        song_id,
                        artifacts.vocals_path,
                        artifacts.accomp_path,
                        artifacts.cache_hit,
                        artifacts.drums_path,
                        artifacts.bass_path,
                        artifacts.other_path,
                    );
                    if let Ok(mut statuses) = separation_statuses.lock() {
                        statuses.insert(song_id.clone(), status);
                    }
                    let _ = app_handle.emit(
                        SEPARATION_COMPLETE_EVENT,
                        SeparationCompleteEvent {
                            song_id: song_id.clone(),
                        },
                    );
                    completed += 1;
                }
                Ok(Err(error)) => {
                    let cmd_error = separation_error(error.to_string());
                    let status = failed_status(song_id, cmd_error.clone());
                    if let Ok(mut statuses) = separation_statuses.lock() {
                        statuses.insert(song_id.clone(), status);
                    }
                    let _ = app_handle.emit(
                        SEPARATION_ERROR_EVENT,
                        SeparationErrorEvent {
                            song_id: song_id.clone(),
                            error: cmd_error,
                        },
                    );
                    failed_count += 1;
                }
                Err(error) => {
                    let cmd_error = separation_error(error.to_string());
                    let status = failed_status(song_id, cmd_error.clone());
                    if let Ok(mut statuses) = separation_statuses.lock() {
                        statuses.insert(song_id.clone(), status);
                    }
                    let _ = app_handle.emit(
                        SEPARATION_ERROR_EVENT,
                        SeparationErrorEvent {
                            song_id: song_id.clone(),
                            error: cmd_error,
                        },
                    );
                    failed_count += 1;
                }
            }
        }

        // Batch complete.
        let _ = app_handle.emit(
            BATCH_SEPARATION_COMPLETE_EVENT,
            BatchSeparationProgress {
                total,
                completed,
                skipped,
                failed: failed_count,
                current_song_id: None,
                current_percent: 0,
            },
        );
        batch_running.store(false, Ordering::Relaxed);
    });

    Ok(())
}

#[tauri::command]
pub fn cancel_batch_separation(state: State<'_, AppState>) -> CommandResult<()> {
    if !state.batch_running.load(Ordering::Relaxed) {
        return Err(separation_error(
            "No batch separation is currently running".to_owned(),
        ));
    }
    state.batch_cancel.store(true, Ordering::Relaxed);
    Ok(())
}
