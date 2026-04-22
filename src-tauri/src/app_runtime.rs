use crate::library_root::LibraryRoot;
use crate::{
    airplay_stream, audio,
    audio::playback::{monotonic_now_ms, PlaybackController, PLAYBACK_POSITION_POLL_INTERVAL_MS},
    cache, commands, config, derive_startup_model_bootstrap, separator, AppState,
};
use std::{
    collections::HashMap,
    fs,
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, AtomicU64},
        Arc, Mutex,
    },
    thread,
    time::Duration,
};
use tauri::{Emitter, Manager, Runtime};

pub fn setup_app<R: Runtime>(app: &mut tauri::App<R>) -> Result<(), Box<dyn std::error::Error>> {
    let app_resource_dir = app.path().resource_dir()?;
    separator::model::ensure_runtime_loaded(Some(&app_resource_dir))?;

    let app_data_dir = app.path().app_data_dir()?;
    fs::create_dir_all(&app_data_dir)?;
    let app_config = config::load_config(&app_data_dir)?;
    let window_shell_state = crate::window_shell::initialize_main_window(app, app_config.as_ref());

    let playback = Arc::new(Mutex::new(PlaybackController::default()));
    let cdg_state: Arc<Mutex<Option<commands::cdg::CdgPlaybackState>>> = Arc::new(Mutex::new(None));
    let airplay_audio_tap = Arc::new(airplay_stream::AirPlayAudioTap::new(12));
    let airplay_stream_generation = Arc::new(AtomicU64::new(1));
    let airplay_audience_active = Arc::new(AtomicBool::new(false));
    let airplay_control_refresh_token = Arc::new(AtomicU64::new(0));
    let airplay_local_output_suppressed = Arc::new(AtomicBool::new(false));
    let model_bootstrap = build_startup_model_bootstrap(&app_data_dir, app_config.as_ref())?;
    let model_bootstrap_status = Arc::new(Mutex::new(model_bootstrap.status.clone()));

    app.manage(window_shell_state);
    app.manage(AppState {
        library: Arc::new(Mutex::new(load_library(app_config.as_ref()))),
        app_data_dir,
        app_resource_dir,
        model_path: model_bootstrap.model_path.clone(),
        playback: Arc::clone(&playback),
        cdg_state: Arc::clone(&cdg_state),
        airplay_audio_tap: Arc::clone(&airplay_audio_tap),
        airplay_stream_generation: Arc::clone(&airplay_stream_generation),
        airplay_audience_active: Arc::clone(&airplay_audience_active),
        airplay_control_refresh_token: Arc::clone(&airplay_control_refresh_token),
        airplay_http_server: Arc::new(Mutex::new(None)),
        airplay_local_output_suppressed: Arc::clone(&airplay_local_output_suppressed),
        playback_request_id: AtomicU64::new(0),
        audio_output_started: Arc::new(AtomicBool::new(false)),
        audio_output_start_lock: Arc::new(Mutex::new(())),
        model_bootstrap_status: Arc::clone(&model_bootstrap_status),
        separation_statuses: Arc::new(Mutex::new(HashMap::new())),
        remote_auth_sessions: Arc::new(Mutex::new(HashMap::new())),
        remote_upload_statuses: Arc::new(Mutex::new(HashMap::new())),
        separator_model_cache: Arc::new(Mutex::new(separator::model_cache::ModelCache::default())),
        batch_running: Arc::new(AtomicBool::new(false)),
        batch_cancel: Arc::new(AtomicBool::new(false)),
    });

    airplay_stream::spawn_audio_forwarder(Arc::clone(&airplay_audio_tap));
    crate::services::playback::spawn_airplay_control_refresh_worker(
        Arc::clone(&airplay_audience_active),
        Arc::clone(&airplay_control_refresh_token),
        Arc::clone(&airplay_audio_tap),
        Arc::clone(&airplay_stream_generation),
    );
    spawn_playback_position_emitter(app.handle().clone(), playback);

    if model_bootstrap.should_spawn_bootstrap_worker {
        spawn_model_bootstrap_worker(
            app.handle().clone(),
            model_bootstrap.managed_model_path,
            model_bootstrap.descriptor,
            model_bootstrap_status,
        );
    }

    Ok(())
}

struct StartupBootstrapResources {
    model_path: PathBuf,
    managed_model_path: PathBuf,
    status: commands::bootstrap::ModelBootstrapStatusSnapshot,
    should_spawn_bootstrap_worker: bool,
    descriptor: &'static separator::bootstrap::ModelDescriptor,
}

fn build_startup_model_bootstrap(
    app_data_dir: &std::path::Path,
    app_config: Option<&config::AppConfig>,
) -> anyhow::Result<StartupBootstrapResources> {
    let active_variant = app_config
        .map(|config| config.effective_model_variant())
        .unwrap_or_default();
    let descriptor = separator::bootstrap::descriptor_for(active_variant);
    let startup_bootstrap = derive_startup_model_bootstrap(
        app_data_dir,
        &separator::model::default_model_path_for_filename(descriptor.filename),
        active_variant,
        descriptor.sha256,
    )?;

    Ok(StartupBootstrapResources {
        model_path: startup_bootstrap.model_path,
        managed_model_path: startup_bootstrap.managed_model_path,
        status: startup_bootstrap.status,
        should_spawn_bootstrap_worker: startup_bootstrap.should_spawn_bootstrap_worker,
        descriptor,
    })
}

fn load_library(app_config: Option<&config::AppConfig>) -> Option<LibraryRoot> {
    let path = app_config
        .and_then(|config| config.active_library())
        .and_then(|library| library.working_copy_root())?;
    let lib_path = PathBuf::from(&path);

    match LibraryRoot::open(&lib_path) {
        Ok(lib) => {
            let db_path = lib.database_path();
            if let Err(err) = cache::initialize_library_database(&db_path) {
                eprintln!(
                    "warning: failed to apply migrations on library at {}: {}",
                    lib_path.display(),
                    err
                );
            }
            Some(lib)
        }
        Err(err) => {
            eprintln!(
                "warning: could not open library at {}: {}",
                lib_path.display(),
                err
            );
            None
        }
    }
}

fn spawn_playback_position_emitter<R: Runtime>(
    app_handle: tauri::AppHandle<R>,
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

            if last_emitted_position != Some(snapshot.position_ms) {
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

pub(crate) fn spawn_model_bootstrap_worker<R: Runtime>(
    app_handle: tauri::AppHandle<R>,
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

        let snapshot = match result {
            Ok(Ok(())) => commands::bootstrap::ready_status(model_path.display().to_string()),
            Ok(Err(error)) => commands::bootstrap::failed_status(
                model_path.display().to_string(),
                commands::error::model_bootstrap_error(error.to_string()),
            ),
            Err(error) => commands::bootstrap::failed_status(
                model_path.display().to_string(),
                commands::error::model_bootstrap_error(error.to_string()),
            ),
        };

        if let Ok(mut current) = status.lock() {
            *current = snapshot.clone();
        }

        let event = match snapshot.state {
            commands::bootstrap::ModelBootstrapState::Ready => {
                commands::bootstrap::MODEL_BOOTSTRAP_READY_EVENT
            }
            commands::bootstrap::ModelBootstrapState::Failed => {
                commands::bootstrap::MODEL_BOOTSTRAP_ERROR_EVENT
            }
            commands::bootstrap::ModelBootstrapState::Pending
            | commands::bootstrap::ModelBootstrapState::Downloading
            | commands::bootstrap::ModelBootstrapState::Outdated => return,
        };
        let _ = app_handle.emit(event, snapshot);
    });
}
