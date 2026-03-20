use crate::{
    commands::error::{internal_error, CommandResult},
    lyrics::parser::LyricLine,
};
use serde::{Deserialize, Serialize};
use std::{
    ffi::{c_char, c_void, CStr, CString},
    path::PathBuf,
    ptr::null,
    sync::{Mutex, OnceLock},
};
use tauri::{AppHandle, Emitter, Manager, WebviewWindow};

pub const AIRPLAY_OUTPUT_STATE_EVENT: &str = "openkara://airplay-output-state";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum AirPlayAudienceMode {
    Idle,
    Lyrics,
    Cdg,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AirPlayRoutePickerBounds {
    pub left: f64,
    pub top: f64,
    pub width: f64,
    pub height: f64,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AirPlayAudienceStatePayload {
    pub mode: AirPlayAudienceMode,
    pub song_id: Option<String>,
    pub is_playing: bool,
    pub position_ms: u64,
    pub lines: Vec<LyricLine>,
    pub active_line_index: i64,
    pub offset_ms: i64,
    pub lyrics_font_step: i8,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct AirPlayOutputStateEvent {
    pub active: bool,
    pub route_name: Option<String>,
    pub mode: AirPlayAudienceMode,
}

#[derive(Default)]
struct AirPlayRuntimeState {
    app_handle: Option<AppHandle>,
    latest_payload: Option<AirPlayAudienceStatePayload>,
}

fn airplay_runtime_state() -> &'static Mutex<AirPlayRuntimeState> {
    static AIRPLAY_RUNTIME_STATE: OnceLock<Mutex<AirPlayRuntimeState>> = OnceLock::new();
    AIRPLAY_RUNTIME_STATE.get_or_init(|| Mutex::new(AirPlayRuntimeState::default()))
}

pub fn normalize_host_y(host_height: f64, dom_top: f64, host_height_for_view: f64) -> f64 {
    host_height - dom_top - host_height_for_view
}

fn mode_tag(mode: AirPlayAudienceMode) -> i32 {
    match mode {
        AirPlayAudienceMode::Idle => 0,
        AirPlayAudienceMode::Lyrics => 1,
        AirPlayAudienceMode::Cdg => 2,
    }
}

fn mode_from_tag(tag: i32) -> AirPlayAudienceMode {
    match tag {
        1 => AirPlayAudienceMode::Lyrics,
        2 => AirPlayAudienceMode::Cdg,
        _ => AirPlayAudienceMode::Idle,
    }
}

fn remember_app_handle(app_handle: &AppHandle) {
    if let Ok(mut state) = airplay_runtime_state().lock() {
        state.app_handle = Some(app_handle.clone());
    }
}

fn emit_airplay_state(active: bool, route_name: Option<String>, mode: AirPlayAudienceMode) {
    let handle = airplay_runtime_state()
        .lock()
        .ok()
        .and_then(|state| state.app_handle.clone());

    if let Some(handle) = handle {
        let _ = handle.emit(
            AIRPLAY_OUTPUT_STATE_EVENT,
            AirPlayOutputStateEvent {
                active,
                route_name,
                mode,
            },
        );
    }
}

fn ensure_placeholder_silence_asset(app_handle: &AppHandle) -> CommandResult<PathBuf> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|error| internal_error(format!("failed to get app data dir: {error}")))?;
    let airplay_dir = app_data_dir.join("airplay");
    std::fs::create_dir_all(&airplay_dir)
        .map_err(|error| internal_error(format!("failed to create airplay dir: {error}")))?;

    let silence_path = airplay_dir.join("placeholder-silence.wav");
    if silence_path.exists() {
        return Ok(silence_path);
    }

    let spec = hound::WavSpec {
        channels: 1,
        sample_rate: 44_100,
        bits_per_sample: 16,
        sample_format: hound::SampleFormat::Int,
    };
    let mut writer = hound::WavWriter::create(&silence_path, spec)
        .map_err(|error| internal_error(format!("failed to create silence asset: {error}")))?;

    for _ in 0..44_100 {
        writer
            .write_sample::<i16>(0)
            .map_err(|error| internal_error(format!("failed to write silence asset: {error}")))?;
    }

    writer
        .finalize()
        .map_err(|error| internal_error(format!("failed to finalize silence asset: {error}")))?;

    Ok(silence_path)
}

#[cfg(target_os = "macos")]
mod native {
    use super::*;
    use std::sync::OnceLock;

    unsafe extern "C" {
        fn ok_airplay_set_state_callback(
            callback: extern "C" fn(bool, *const c_char, i32),
        );
        fn ok_airplay_sync_route_picker(
            ns_view_ptr: *mut c_void,
            left: f64,
            top: f64,
            width: f64,
            height: f64,
            mounted: bool,
            silence_asset_path: *const c_char,
        );
        fn ok_airplay_update_mode(
            mode: i32,
            placeholder_enabled: bool,
            silence_asset_path: *const c_char,
        );
    }

    extern "C" fn handle_airplay_state_callback(
        active: bool,
        route_name: *const c_char,
        mode: i32,
    ) {
        let route_name = if route_name.is_null() {
            None
        } else {
            // SAFETY: Objective-C bridge passes a valid UTF-8 string pointer or NULL.
            Some(
                unsafe { CStr::from_ptr(route_name) }
                    .to_string_lossy()
                    .into_owned(),
            )
        };

        emit_airplay_state(active, route_name, mode_from_tag(mode));
    }

    pub(super) fn ensure_callback_registered() {
        static CALLBACK_REGISTERED: OnceLock<()> = OnceLock::new();
        CALLBACK_REGISTERED.get_or_init(|| {
            // SAFETY: The callback is a process-global function with static lifetime.
            unsafe { ok_airplay_set_state_callback(handle_airplay_state_callback) };
        });
    }

    pub(super) fn sync_route_picker(
        window: &WebviewWindow,
        silence_asset_path: &PathBuf,
        bounds: Option<AirPlayRoutePickerBounds>,
    ) -> CommandResult<()> {
        ensure_callback_registered();

        let silence_asset_path = CString::new(silence_asset_path.to_string_lossy().as_bytes())
            .map_err(|error| internal_error(format!("invalid silence asset path: {error}")))?;

        match bounds {
            Some(bounds) => {
                let ns_view_ptr = window
                    .ns_view()
                    .map_err(|error| internal_error(format!("failed to get ns_view: {error}")))?;

                // SAFETY: Tauri provides the active NSView pointer for the webview window.
                unsafe {
                    ok_airplay_sync_route_picker(
                        ns_view_ptr,
                        bounds.left,
                        bounds.top,
                        bounds.width,
                        bounds.height,
                        true,
                        silence_asset_path.as_ptr(),
                    );
                }
            }
            None => {
                // SAFETY: A null pointer with mounted=false is the bridge teardown path.
                unsafe {
                    ok_airplay_sync_route_picker(
                        std::ptr::null_mut(),
                        0.0,
                        0.0,
                        0.0,
                        0.0,
                        false,
                        null(),
                    );
                }
            }
        }

        Ok(())
    }

    pub(super) fn update_mode(
        mode: AirPlayAudienceMode,
        placeholder_enabled: bool,
        silence_asset_path: &PathBuf,
    ) -> CommandResult<()> {
        ensure_callback_registered();

        let silence_asset_path = CString::new(silence_asset_path.to_string_lossy().as_bytes())
            .map_err(|error| internal_error(format!("invalid silence asset path: {error}")))?;

        // SAFETY: The bridge copies the C string before returning and dispatches to the main thread.
        unsafe {
            ok_airplay_update_mode(
                mode_tag(mode),
                placeholder_enabled,
                silence_asset_path.as_ptr(),
            );
        }

        Ok(())
    }
}

#[cfg(not(target_os = "macos"))]
mod native {
    use super::*;

    pub(super) fn sync_route_picker(
        _window: &WebviewWindow,
        _silence_asset_path: &PathBuf,
        _bounds: Option<AirPlayRoutePickerBounds>,
    ) -> CommandResult<()> {
        Ok(())
    }

    pub(super) fn update_mode(
        _mode: AirPlayAudienceMode,
        _placeholder_enabled: bool,
        _silence_asset_path: &PathBuf,
    ) -> CommandResult<()> {
        Ok(())
    }
}

#[tauri::command]
pub fn sync_airplay_route_picker(
    app_handle: AppHandle,
    window: WebviewWindow,
    bounds: Option<AirPlayRoutePickerBounds>,
) -> CommandResult<()> {
    remember_app_handle(&app_handle);
    let silence_asset_path = ensure_placeholder_silence_asset(&app_handle)?;
    native::sync_route_picker(&window, &silence_asset_path, bounds)
}

#[tauri::command]
pub fn sync_airplay_audience_state(
    app_handle: AppHandle,
    payload: AirPlayAudienceStatePayload,
) -> CommandResult<()> {
    remember_app_handle(&app_handle);

    if let Ok(mut state) = airplay_runtime_state().lock() {
        state.latest_payload = Some(payload.clone());
    }

    let silence_asset_path = ensure_placeholder_silence_asset(&app_handle)?;
    let placeholder_enabled =
        payload.mode != AirPlayAudienceMode::Idle && payload.song_id.is_some();
    native::update_mode(payload.mode, placeholder_enabled, &silence_asset_path)
}
