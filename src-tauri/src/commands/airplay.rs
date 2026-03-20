use crate::{
    airplay_stream::{default_stream_root, AirPlayHttpServer},
    commands::cdg::render_cdg_frame_bytes,
    commands::error::{internal_error, CommandResult},
    AppState,
    lyrics::parser::LyricLine,
};
use serde::{Deserialize, Serialize};
use std::{
    ffi::{c_char, c_void, CStr, CString},
    path::PathBuf,
    ptr::null,
    sync::{Mutex, OnceLock},
};
use tauri::{AppHandle, Emitter, State, WebviewWindow};

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
pub struct AirPlayViewport {
    pub width_px: u32,
    pub height_px: u32,
    pub bottom_inset_px: u32,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AirPlayAudienceMessages {
    pub select_song: String,
    pub loading_lyrics: String,
    pub no_lyrics: String,
    pub add_lyrics: String,
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
    pub is_loading: bool,
    pub lyrics_font_step: i8,
    pub messages: AirPlayAudienceMessages,
    pub viewport: AirPlayViewport,
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

fn ensure_stream_server(state: &AppState) -> CommandResult<(PathBuf, String)> {
    let root_dir = default_stream_root(&state.app_data_dir);
    let mut server = state
        .airplay_http_server
        .lock()
        .map_err(|_| internal_error("airplay http server lock was poisoned".to_owned()))?;

    if server.is_none() {
        *server = Some(
            AirPlayHttpServer::bind(&root_dir)
                .map_err(|error| internal_error(format!("failed to start airplay server: {error}")))?,
        );
    }

    let base_url = server
        .as_ref()
        .map(|server| server.base_url().to_owned())
        .ok_or_else(|| internal_error("airplay server handle was not initialized".to_owned()))?;

    Ok((root_dir, format!("{base_url}/playlist.m3u8")))
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
            stream_root_path: *const c_char,
            playlist_url: *const c_char,
        );
        fn ok_airplay_sync_audience_state(
            mode: i32,
            scene_json: *const c_char,
            cdg_frame_ptr: *const u8,
            cdg_frame_len: usize,
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
        stream_root: &PathBuf,
        playlist_url: &str,
        bounds: Option<AirPlayRoutePickerBounds>,
    ) -> CommandResult<()> {
        ensure_callback_registered();

        let stream_root = CString::new(stream_root.to_string_lossy().as_bytes())
            .map_err(|error| internal_error(format!("invalid airplay stream root: {error}")))?;
        let playlist_url = CString::new(playlist_url.as_bytes())
            .map_err(|error| internal_error(format!("invalid airplay playlist url: {error}")))?;

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
                        stream_root.as_ptr(),
                        playlist_url.as_ptr(),
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
                        null(),
                    );
                }
            }
        }

        Ok(())
    }

    pub(super) fn sync_audience_state(
        payload: &AirPlayAudienceStatePayload,
        cdg_frame: Option<&[u8]>,
    ) -> CommandResult<()> {
        ensure_callback_registered();

        let scene_json = serde_json::to_string(payload)
            .map_err(|error| internal_error(format!("failed to serialize airplay scene: {error}")))?;
        let scene_json = CString::new(scene_json.as_bytes())
            .map_err(|error| internal_error(format!("invalid airplay scene json: {error}")))?;
        let (cdg_frame_ptr, cdg_frame_len) = match cdg_frame {
            Some(frame) => (frame.as_ptr(), frame.len()),
            None => (null::<u8>(), 0),
        };

        // SAFETY: The bridge copies any provided data before returning and dispatches work onto its own queues.
        unsafe {
            ok_airplay_sync_audience_state(
                mode_tag(payload.mode),
                scene_json.as_ptr(),
                cdg_frame_ptr,
                cdg_frame_len,
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
        _stream_root: &PathBuf,
        _playlist_url: &str,
        _bounds: Option<AirPlayRoutePickerBounds>,
    ) -> CommandResult<()> {
        Ok(())
    }

    pub(super) fn sync_audience_state(
        _payload: &AirPlayAudienceStatePayload,
        _cdg_frame: Option<&[u8]>,
    ) -> CommandResult<()> {
        Ok(())
    }
}

#[tauri::command]
pub fn sync_airplay_route_picker(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    window: WebviewWindow,
    bounds: Option<AirPlayRoutePickerBounds>,
) -> CommandResult<()> {
    remember_app_handle(&app_handle);
    let (stream_root, playlist_url) = ensure_stream_server(&state)?;
    native::sync_route_picker(&window, &stream_root, &playlist_url, bounds)
}

#[tauri::command]
pub fn sync_airplay_audience_state(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    payload: AirPlayAudienceStatePayload,
) -> CommandResult<()> {
    remember_app_handle(&app_handle);

    if let Ok(mut state) = airplay_runtime_state().lock() {
        state.latest_payload = Some(payload.clone());
    }

    let cdg_frame = if payload.mode == AirPlayAudienceMode::Cdg {
        let mut cdg_state = state
            .cdg_state
            .lock()
            .map_err(|_| internal_error("CDG state lock was poisoned".to_owned()))?;
        render_cdg_frame_bytes(&mut cdg_state, payload.position_ms)
    } else {
        None
    };

    native::sync_audience_state(&payload, cdg_frame.as_deref())
}
