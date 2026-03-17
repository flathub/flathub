pub mod audio;
pub mod cache;
pub mod cdg;
pub mod commands;
pub mod config;
pub mod library;
pub mod library_root;
pub mod lyrics;
pub mod metadata;
pub mod perf;
pub mod separator;
pub mod smoke;
use crate::audio::playback::{
    monotonic_now_ms, PlaybackController, PLAYBACK_POSITION_POLL_INTERVAL_MS,
};
use crate::library_root::LibraryRoot;
use std::{
    collections::HashMap,
    fs,
    path::{Path, PathBuf},
    sync::atomic::{AtomicBool, AtomicU64},
    sync::{Arc, Mutex},
    thread,
    time::Duration,
};
use tauri::{Emitter, Manager};

pub struct AppState {
    /// The active karaoke library. `None` if no library has been configured yet
    /// (first-run state).
    pub library: Arc<Mutex<Option<LibraryRoot>>>,
    /// Per-machine app data directory (stores config.json and AI model).
    pub app_data_dir: PathBuf,
    pub model_path: PathBuf,
    pub playback: Arc<Mutex<PlaybackController>>,
    pub cdg_state: Arc<Mutex<Option<commands::cdg::CdgPlaybackState>>>,
    pub playback_request_id: AtomicU64,
    pub audio_output_started: Arc<AtomicBool>,
    pub audio_output_start_lock: Arc<Mutex<()>>,
    pub model_bootstrap_status: Arc<Mutex<commands::bootstrap::ModelBootstrapStatusSnapshot>>,
    pub separation_statuses:
        Arc<Mutex<HashMap<String, commands::separation::SeparationStatusSnapshot>>>,
    pub batch_running: Arc<AtomicBool>,
    pub batch_cancel: Arc<AtomicBool>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct StartupModelBootstrapPlan {
    pub model_path: PathBuf,
    pub managed_model_path: PathBuf,
    pub status: commands::bootstrap::ModelBootstrapStatusSnapshot,
    pub should_spawn_bootstrap_worker: bool,
}

impl AppState {
    /// Convenience: get a clone of the LibraryRoot (if configured).
    pub fn library_root(&self) -> Result<LibraryRoot, commands::error::CommandError> {
        let guard = self
            .library
            .lock()
            .map_err(|_| commands::error::state_lock_error("library lock was poisoned"))?;
        guard
            .clone()
            .ok_or_else(|| commands::error::library_error("no library configured".to_owned()))
    }

    /// Resolve the path to the active AI model based on the current config.
    ///
    /// Checks (in order): managed model dir for the active variant, then dev fallback.
    ///
    /// This must stay variant-aware. Falling back to a single hard-coded model
    /// filename can silently run the wrong separator after users switch quality modes.
    pub fn resolve_model_path(&self) -> Result<PathBuf, commands::error::CommandError> {
        let variant = config::load_config(&self.app_data_dir)
            .ok()
            .flatten()
            .map(|c| c.effective_model_variant())
            .unwrap_or_default();
        let descriptor = separator::bootstrap::descriptor_for(variant);
        let managed =
            separator::bootstrap::managed_model_path_for(&self.app_data_dir, descriptor);
        let dev_path = separator::model::default_model_path_for_filename(descriptor.filename);
        let resolved = separator::bootstrap::resolve_existing_model_path(
            &managed,
            &dev_path,
            descriptor.sha256,
        )
        .map_err(|error| commands::error::internal_error(error.to_string()))?;

        Ok(resolved
            .map(|resolved| resolved.path)
            .unwrap_or(managed))
    }
}

pub fn derive_startup_model_bootstrap(
    app_data_dir: &Path,
    development_model_path: &Path,
    active_variant: config::ModelVariant,
    expected_sha256: &str,
) -> anyhow::Result<StartupModelBootstrapPlan> {
    // Startup readiness must mean "the active variant has a verified model",
    // not merely "some file exists at the managed path". That distinction is
    // what prevents re-downloading already-installed models on every launch.
    let descriptor = separator::bootstrap::descriptor_for(active_variant);
    let managed_model_path =
        separator::bootstrap::managed_model_path_for(app_data_dir, descriptor);
    let resolved_model = separator::bootstrap::resolve_existing_model_path(
        &managed_model_path,
        development_model_path,
        expected_sha256,
    )?;
    let model_path = resolved_model
        .as_ref()
        .map(|resolved| resolved.path.clone())
        .unwrap_or_else(|| managed_model_path.clone());
    let status = match resolved_model.as_ref() {
        Some(resolved) => commands::bootstrap::ready_status(resolved.path.display().to_string()),
        None => commands::bootstrap::pending_status(managed_model_path.display().to_string()),
    };
    let should_spawn_bootstrap_worker = resolved_model.is_none();

    Ok(StartupModelBootstrapPlan {
        model_path,
        managed_model_path,
        status,
        should_spawn_bootstrap_worker,
    })
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            let app_data_dir = app
                .path()
                .app_data_dir()
                .map_err(|error| -> Box<dyn std::error::Error> { error.into() })?;
            fs::create_dir_all(&app_data_dir)
                .map_err(|error| -> Box<dyn std::error::Error> { error.into() })?;

            // Load per-machine config to find the library path.
            let app_config = config::load_config(&app_data_dir)
                .map_err(|error| -> Box<dyn std::error::Error> { error.into() })?;

            let library = match app_config.as_ref().and_then(|config| config.library_path.clone()) {
                Some(path) => {
                    let lib_path = PathBuf::from(&path);
                    match LibraryRoot::open(&lib_path) {
                        Ok(lib) => {
                            // Run migrations on the library database.
                            let db_path = lib.database_path();
                            if let Err(err) = cache::initialize_library_database(&db_path) {
                                eprintln!(
                                    "warning: failed to apply migrations on library at {}: {}",
                                    path, err
                                );
                            }
                            Some(lib)
                        }
                        Err(err) => {
                            eprintln!(
                                "warning: could not open library at {}: {}",
                                path, err
                            );
                            None
                        }
                    }
                }
                None => None,
            };

            let playback = Arc::new(Mutex::new(PlaybackController::default()));
            let cdg_state = Arc::new(Mutex::new(None));
            let audio_output_started = Arc::new(AtomicBool::new(false));
            let audio_output_start_lock = Arc::new(Mutex::new(()));
            let active_variant = app_config
                .as_ref()
                .map(|config| config.effective_model_variant())
                .unwrap_or_default();
            let descriptor = separator::bootstrap::descriptor_for(active_variant);
            let startup_bootstrap = derive_startup_model_bootstrap(
                &app_data_dir,
                &separator::model::default_model_path_for_filename(descriptor.filename),
                active_variant,
                descriptor.sha256,
            )
            .map_err(|error| -> Box<dyn std::error::Error> { error.into() })?;
            let model_path = startup_bootstrap.model_path.clone();
            let model_bootstrap_status = Arc::new(Mutex::new(startup_bootstrap.status.clone()));
            let separation_statuses = Arc::new(Mutex::new(HashMap::new()));

            app.manage(AppState {
                library: Arc::new(Mutex::new(library)),
                app_data_dir,
                model_path: model_path.clone(),
                playback: Arc::clone(&playback),
                cdg_state,
                playback_request_id: AtomicU64::new(0),
                audio_output_started,
                audio_output_start_lock,
                model_bootstrap_status: Arc::clone(&model_bootstrap_status),
                separation_statuses,
                batch_running: Arc::new(AtomicBool::new(false)),
                batch_cancel: Arc::new(AtomicBool::new(false)),
            });
            spawn_playback_position_emitter(app.handle().clone(), playback);
            // Only auto-download when startup could not verify a ready model for
            // the active variant. Re-triggering whenever the managed path is the
            // selected path caused redundant downloads on every app launch.
            if startup_bootstrap.should_spawn_bootstrap_worker {
                spawn_model_bootstrap_worker(
                    app.handle().clone(),
                    startup_bootstrap.managed_model_path,
                    descriptor,
                    model_bootstrap_status,
                );
            }

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::bootstrap::get_model_bootstrap_status,
            commands::import::import_songs,
            commands::import::get_library,
            commands::import::search_library,
            commands::import::update_song_metadata,
            commands::import::get_song_properties,
            commands::library_setup::create_library,
            commands::library_setup::open_library,
            commands::library_setup::get_library_path,
            commands::lyrics::fetch_lyrics,
            commands::lyrics::set_lyrics_offset,
            commands::lyrics::save_manual_lyrics,
            commands::lyrics::import_lyrics_files,
            commands::lyrics::extract_embedded_lyrics,
            commands::lyrics::fetch_lyrics_online,
            commands::maintenance::delete_all_stems,
            commands::maintenance::estimate_stems_size,
            commands::maintenance::delete_all_cached_lyrics,
            commands::maintenance::downgrade_all_to_two_stem,
            commands::maintenance::estimate_downgrade_savings,
            commands::playback::play,
            commands::playback::resume,
            commands::playback::pause,
            commands::playback::seek,
            commands::playback::set_volume,
            commands::playback::set_stem_volume,
            commands::playback::load_stems,
            commands::playback::get_playback_state,
            commands::cdg::get_cdg_frame,
            commands::batch_separation::batch_separate,
            commands::batch_separation::cancel_batch_separation,
            commands::separation::separate,
            commands::separation::get_separation_status,
            commands::separation::get_all_separation_statuses,
            commands::separation::upgrade_to_four_stem,
            commands::separation::re_separate,
            commands::separation::downgrade_single_to_two_stem,
            commands::settings::get_settings,
            commands::settings::set_stem_mode,
            commands::settings::set_model_variant,
            commands::settings::set_language,
            commands::settings::set_hide_batch_separate,
            commands::bootstrap::download_model,
            commands::bootstrap::delete_model,
            commands::bootstrap::get_model_status
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

fn spawn_playback_position_emitter(
    app_handle: tauri::AppHandle,
    playback: Arc<Mutex<PlaybackController>>,
) {
    thread::spawn(move || {
        let mut last_emitted_position = None;
        let mut was_playing = false;
        let mut last_song_id: Option<String> = None;

        loop {
            thread::sleep(Duration::from_millis(PLAYBACK_POSITION_POLL_INTERVAL_MS));

            let snapshot = match playback.lock() {
                Ok(mut controller) => controller.snapshot(monotonic_now_ms()),
                Err(_) => break,
            };

            // Detect natural song end: was playing, now stopped, position reached duration
            if was_playing
                && !snapshot.is_playing
                && snapshot
                    .duration_ms
                    .map_or(false, |d| snapshot.position_ms >= d)
                && last_song_id.is_some()
                && last_song_id == snapshot.song_id
            {
                if let Some(ref song_id) = snapshot.song_id {
                    let _ = app_handle.emit(
                        audio::playback::PLAYBACK_ENDED_EVENT,
                        audio::playback::PlaybackEndedEvent {
                            song_id: song_id.clone(),
                        },
                    );
                }
            }

            was_playing = snapshot.is_playing;
            last_song_id = snapshot.song_id.clone();

            if snapshot.song_id.is_none() {
                last_emitted_position = None;
                continue;
            }

            let should_emit = last_emitted_position != Some(snapshot.position_ms);
            if should_emit {
                let _ = app_handle.emit(
                    audio::playback::PLAYBACK_POSITION_EVENT,
                    audio::playback::PlaybackPositionEvent {
                        ms: snapshot.position_ms,
                    },
                );
                last_emitted_position = Some(snapshot.position_ms);
            }
        }
    });
}

fn spawn_model_bootstrap_worker(
    app_handle: tauri::AppHandle,
    model_path: PathBuf,
    descriptor: &'static separator::bootstrap::ModelDescriptor,
    status: Arc<Mutex<commands::bootstrap::ModelBootstrapStatusSnapshot>>,
) {
    let progress_path = model_path.display().to_string();
    tauri::async_runtime::spawn(async move {
        let blocking_status = Arc::clone(&status);
        let blocking_app_handle = app_handle.clone();
        let blocking_model_path = model_path.clone();
        let progress_path = progress_path.clone();

        let result = tauri::async_runtime::spawn_blocking(move || {
            separator::bootstrap::download_and_install_model(
                &blocking_model_path,
                descriptor.download_url,
                descriptor.sha256,
                |downloaded_bytes, total_bytes| {
                    let snapshot = commands::bootstrap::downloading_status(
                        progress_path.clone(),
                        downloaded_bytes,
                        total_bytes,
                    );
                    if let Ok(mut current) = blocking_status.lock() {
                        *current = snapshot.clone();
                    }
                    let _ = blocking_app_handle.emit(
                        commands::bootstrap::MODEL_BOOTSTRAP_PROGRESS_EVENT,
                        snapshot,
                    );
                },
            )
        })
        .await;

        match result {
            Ok(Ok(())) => {
                let snapshot = commands::bootstrap::ready_status(model_path.display().to_string());
                if let Ok(mut current) = status.lock() {
                    *current = snapshot.clone();
                }
                let _ = app_handle.emit(commands::bootstrap::MODEL_BOOTSTRAP_READY_EVENT, snapshot);
            }
            Ok(Err(error)) => {
                let command_error = commands::error::model_bootstrap_error(error.to_string());
                let snapshot = commands::bootstrap::failed_status(
                    model_path.display().to_string(),
                    command_error,
                );
                if let Ok(mut current) = status.lock() {
                    *current = snapshot.clone();
                }
                let _ = app_handle.emit(commands::bootstrap::MODEL_BOOTSTRAP_ERROR_EVENT, snapshot);
            }
            Err(error) => {
                let command_error = commands::error::model_bootstrap_error(error.to_string());
                let snapshot = commands::bootstrap::failed_status(
                    model_path.display().to_string(),
                    command_error,
                );
                if let Ok(mut current) = status.lock() {
                    *current = snapshot.clone();
                }
                let _ = app_handle.emit(commands::bootstrap::MODEL_BOOTSTRAP_ERROR_EVENT, snapshot);
            }
        }
    });
}
