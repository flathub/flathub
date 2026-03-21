mod app_menu;
mod app_runtime;
pub mod airplay_stream;
pub mod audio;
pub mod cache;
pub mod cdg;
pub mod commands;
pub mod config;
pub mod library;
pub mod library_root;
pub mod lyrics;
pub mod media_g;
pub mod metadata;
pub mod perf;
pub mod separator;
pub mod services;
pub mod smoke;
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
    pub model_path: PathBuf,
    pub playback: Arc<Mutex<PlaybackController>>,
    pub cdg_state: Arc<Mutex<Option<commands::cdg::CdgPlaybackState>>>,
    pub airplay_audio_tap: Arc<airplay_stream::AirPlayAudioTap>,
    pub airplay_stream_generation: Arc<AtomicU64>,
    pub airplay_http_server: Arc<Mutex<Option<airplay_stream::AirPlayHttpServer>>>,
    pub airplay_local_output_suppressed: Arc<AtomicBool>,
    pub playback_request_id: AtomicU64,
    pub audio_output_started: Arc<AtomicBool>,
    pub audio_output_start_lock: Arc<Mutex<()>>,
    pub model_bootstrap_status: Arc<Mutex<commands::bootstrap::ModelBootstrapStatusSnapshot>>,
    pub separation_statuses:
        Arc<Mutex<HashMap<String, commands::separation::SeparationStatusSnapshot>>>,
    pub separator_model_cache: Arc<Mutex<ModelCache<LoadedModel>>>,
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
        let managed = separator::bootstrap::managed_model_path_for(&self.app_data_dir, descriptor);
        let dev_path = separator::model::default_model_path_for_filename(descriptor.filename);
        let resolved = separator::bootstrap::resolve_existing_model_path(
            &managed,
            &dev_path,
            descriptor.sha256,
        )
        .map_err(|error| commands::error::internal_error(error.to_string()))?;

        Ok(resolved.map(|resolved| resolved.path).unwrap_or(managed))
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
    let builder = tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| app_runtime::setup_app(app))
        .invoke_handler(tauri::generate_handler![
            commands::bootstrap::get_model_bootstrap_status,
            commands::import::import_songs,
            commands::import::get_import_candidate_details,
            commands::import::get_library,
            commands::import::search_library,
            commands::import::delete_songs,
            commands::import::extract_embedded_cover_art,
            commands::import::update_song_metadata,
            commands::import::set_songs_instrumental,
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
            commands::airplay::sync_airplay_route_picker,
            commands::airplay::sync_airplay_audience_state,
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
            commands::settings::set_lyrics_font_step,
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
