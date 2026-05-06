pub mod airplay_stream;
mod app_menu;
mod app_runtime;
pub mod audio;
pub mod cache;
pub mod cdg;
pub mod commands;
pub mod config;
pub mod hash;
pub mod library;
pub mod library_root;
pub mod lyrics;
pub mod media_g;
pub mod metadata;
pub mod perf;
pub mod separator;
pub mod services;
pub mod smoke;
pub mod system_credentials;
mod window_shell;
use crate::audio::playback::PlaybackController;
use crate::library_root::LibraryRoot;
use crate::separator::{model::LoadedModel, model_cache::ModelCache};
use std::{
    collections::HashMap,
    path::{Path, PathBuf},
    sync::atomic::{AtomicBool, AtomicU64},
    sync::{Arc, Mutex},
};

pub struct AppState {
    /// The active karaoke library. `None` if no library has been configured yet
    /// (first-run state).
    pub library: Arc<Mutex<Option<LibraryRoot>>>,
    /// Per-machine app data directory (stores config.json and AI model).
    pub app_data_dir: PathBuf,
    /// App-bundled runtime resources such as packaged OAuth client metadata.
    pub app_resource_dir: PathBuf,
    pub model_path: PathBuf,
    pub playback: Arc<Mutex<PlaybackController>>,
    pub cdg_state: Arc<Mutex<Option<commands::cdg::CdgPlaybackState>>>,
    pub airplay_audio_tap: Arc<airplay_stream::AirPlayAudioTap>,
    pub airplay_stream_generation: Arc<AtomicU64>,
    pub airplay_audience_active: Arc<AtomicBool>,
    pub airplay_control_refresh_token: Arc<AtomicU64>,
    pub airplay_http_server: Arc<Mutex<Option<airplay_stream::AirPlayHttpServer>>>,
    pub airplay_local_output_suppressed: Arc<AtomicBool>,
    pub playback_request_id: Arc<AtomicU64>,
    pub audio_output_started: Arc<AtomicBool>,
    pub audio_output_start_lock: Arc<Mutex<()>>,
    pub model_bootstrap_status: Arc<Mutex<commands::bootstrap::ModelBootstrapStatusSnapshot>>,
    pub separation_statuses:
        Arc<Mutex<HashMap<String, commands::separation::SeparationStatusSnapshot>>>,
    pub remote_auth_sessions:
        Arc<Mutex<HashMap<String, commands::remote_library::RemoteAuthSession>>>,
    pub remote_upload_statuses:
        Arc<Mutex<HashMap<String, commands::remote_library::UploadStatusSnapshot>>>,
    pub separator_model_cache: Arc<Mutex<ModelCache<LoadedModel>>>,
    pub batch_running: Arc<AtomicBool>,
    pub batch_cancel: Arc<AtomicBool>,
    pub shutdown: Arc<AtomicBool>,
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        airplay_stream::AirPlayAudioTap, commands::bootstrap, separator::model_cache::ModelCache,
    };
    use std::sync::atomic::Ordering;

    #[test]
    fn background_app_state_shares_playback_request_generation() {
        let state = AppState {
            library: Arc::new(Mutex::new(None)),
            app_data_dir: PathBuf::from("app-data"),
            app_resource_dir: PathBuf::from("resources"),
            model_path: PathBuf::from("model.bin"),
            playback: Arc::new(Mutex::new(audio::playback::PlaybackController::default())),
            cdg_state: Arc::new(Mutex::new(None)),
            airplay_audio_tap: Arc::new(AirPlayAudioTap::new(4)),
            airplay_stream_generation: Arc::new(AtomicU64::new(1)),
            airplay_audience_active: Arc::new(AtomicBool::new(false)),
            airplay_control_refresh_token: Arc::new(AtomicU64::new(0)),
            airplay_http_server: Arc::new(Mutex::new(None)),
            airplay_local_output_suppressed: Arc::new(AtomicBool::new(false)),
            playback_request_id: Arc::new(AtomicU64::new(41)),
            audio_output_started: Arc::new(AtomicBool::new(false)),
            audio_output_start_lock: Arc::new(Mutex::new(())),
            model_bootstrap_status: Arc::new(Mutex::new(bootstrap::pending_status("model.bin"))),
            separation_statuses: Arc::new(Mutex::new(HashMap::new())),
            remote_auth_sessions: Arc::new(Mutex::new(HashMap::new())),
            remote_upload_statuses: Arc::new(Mutex::new(HashMap::new())),
            separator_model_cache: Arc::new(Mutex::new(ModelCache::default())),
            batch_running: Arc::new(AtomicBool::new(false)),
            batch_cancel: Arc::new(AtomicBool::new(false)),
            shutdown: Arc::new(AtomicBool::new(false)),
        };
        let background = state.clone_for_background();

        state.playback_request_id.fetch_add(1, Ordering::SeqCst);

        assert_eq!(background.playback_request_id.load(Ordering::SeqCst), 42);
    }
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

    /// Produce a lightweight clone suitable for a background thread. Only
    /// the fields needed by publish and download operations are shared.
    pub fn clone_for_background(&self) -> Self {
        Self {
            library: self.library.clone(),
            app_data_dir: self.app_data_dir.clone(),
            remote_upload_statuses: self.remote_upload_statuses.clone(),
            remote_auth_sessions: self.remote_auth_sessions.clone(),
            model_path: self.model_path.clone(),
            playback: self.playback.clone(),
            cdg_state: self.cdg_state.clone(),
            airplay_audio_tap: self.airplay_audio_tap.clone(),
            airplay_stream_generation: self.airplay_stream_generation.clone(),
            airplay_audience_active: self.airplay_audience_active.clone(),
            airplay_control_refresh_token: self.airplay_control_refresh_token.clone(),
            airplay_http_server: self.airplay_http_server.clone(),
            airplay_local_output_suppressed: self.airplay_local_output_suppressed.clone(),
            playback_request_id: self.playback_request_id.clone(),
            audio_output_started: self.audio_output_started.clone(),
            audio_output_start_lock: self.audio_output_start_lock.clone(),
            model_bootstrap_status: self.model_bootstrap_status.clone(),
            separation_statuses: self.separation_statuses.clone(),
            separator_model_cache: self.separator_model_cache.clone(),
            batch_running: self.batch_running.clone(),
            batch_cancel: self.batch_cancel.clone(),
            shutdown: self.shutdown.clone(),
            app_resource_dir: self.app_resource_dir.clone(),
        }
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
        let managed = separator::bootstrap::managed_model_path_for(&self.app_data_dir, descriptor);
        let dev_path = separator::model::default_model_path_for_filename(descriptor.filename);
        match separator::bootstrap::resolve_model_installation(
            &managed,
            &dev_path,
            descriptor.sha256,
        )
        .map_err(|error| commands::error::internal_error(error.to_string()))?
        {
            separator::bootstrap::ModelInstallationResolution::Ready(resolved) => {
                Ok(resolved.path)
            }
            separator::bootstrap::ModelInstallationResolution::LegacyManaged(_) => Err(
                commands::error::model_bootstrap_error(
                    "installed model does not match the pinned release; open Settings to delete it and download the update"
                        .to_string(),
                ),
            ),
            separator::bootstrap::ModelInstallationResolution::Absent => Err(
                commands::error::model_bootstrap_error(
                    "model is not installed or is still downloading".to_string(),
                ),
            ),
        }
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
    let managed_model_path = separator::bootstrap::managed_model_path_for(app_data_dir, descriptor);
    let resolution = separator::bootstrap::resolve_model_installation(
        &managed_model_path,
        development_model_path,
        expected_sha256,
    )?;
    let (model_path, status, should_spawn_bootstrap_worker) = match resolution {
        separator::bootstrap::ModelInstallationResolution::Ready(resolved) => (
            resolved.path.clone(),
            commands::bootstrap::ready_status(resolved.path.display().to_string()),
            false,
        ),
        separator::bootstrap::ModelInstallationResolution::LegacyManaged(path) => (
            path.clone(),
            commands::bootstrap::outdated_status(path.display().to_string()),
            false,
        ),
        separator::bootstrap::ModelInstallationResolution::Absent => (
            managed_model_path.clone(),
            commands::bootstrap::pending_status(managed_model_path.display().to_string()),
            true,
        ),
    };

    Ok(StartupModelBootstrapPlan {
        model_path,
        managed_model_path,
        status,
        should_spawn_bootstrap_worker,
    })
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let builder = tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| app_runtime::setup_app(app))
        .invoke_handler(tauri::generate_handler![
            commands::bootstrap::get_model_bootstrap_status,
            commands::import::import_songs,
            commands::import::get_import_candidate_details,
            commands::import::expand_import_paths,
            commands::import::pick_import_paths,
            commands::import::get_library,
            commands::import::search_library,
            commands::import::delete_songs,
            commands::import::extract_embedded_cover_art,
            commands::import::update_song_metadata,
            commands::import::set_songs_instrumental,
            commands::import::get_song_properties,
            commands::library_setup::create_library,
            commands::library_setup::open_library,
            commands::library_setup::switch_library,
            commands::library_setup::get_library_path,
            commands::library_setup::get_library_registry,
            commands::library_setup::get_active_library,
            commands::library_setup::remove_library,
            commands::library_setup::rename_library,
            commands::library_setup::delete_library,
            commands::remote_library::begin_remote_auth,
            commands::remote_library::poll_remote_auth,
            commands::remote_library::cancel_remote_auth,
            commands::remote_library::open_external_url,
            commands::remote_library::list_remote_library_roots,
            commands::remote_library::create_remote_library,
            commands::remote_library::resolve_remote_library_candidate,
            commands::remote_library::register_remote_library,
            commands::remote_library::reauthorize_remote_library,
            commands::remote_library::mirror_local_library_to_remote,
            commands::remote_library::sync_active_remote_library,
            commands::remote_library::publish_song_to_remote,
            commands::remote_library::publish_songs_to_remote,
            commands::remote_library::get_all_upload_statuses,
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
            commands::airplay::sync_airplay_route_picker,
            commands::airplay::sync_airplay_audience_state,
            commands::airplay::step_airplay_plain_text_page,
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
            commands::settings::set_cover_art_backdrop,
            commands::settings::set_lyrics_font_step,
            commands::settings::set_execution_provider,
            commands::settings::restart_app,
            commands::window_shell::get_window_shell_state,
            commands::window_shell::set_native_sidebar_visibility,
            commands::bootstrap::download_model,
            commands::bootstrap::delete_model,
            commands::bootstrap::get_model_status
        ]);

    #[cfg(target_os = "macos")]
    let builder = builder
        .menu(|app| app_menu::build_app_menu(app))
        .on_menu_event(app_menu::handle_menu_event);

    builder
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
