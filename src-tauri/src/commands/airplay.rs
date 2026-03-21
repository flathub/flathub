use crate::{
    airplay_stream::stream_tick_interval,
    airplay_stream::{default_stream_root, AirPlayHttpServer},
    audio::playback::{monotonic_now_ms, PlaybackController, PlaybackStateSnapshot},
    commands::cdg::{render_cdg_frame_bytes, CdgPlaybackState},
    commands::error::{internal_error, CommandResult},
    lyrics::parser::LyricLine,
    AppState,
};
use serde::{Deserialize, Serialize};
use std::{
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, AtomicU64, Ordering},
        Arc, Mutex, OnceLock,
    },
    thread,
};
use tauri::{AppHandle, Emitter, State, WebviewWindow};

#[cfg(target_os = "macos")]
use std::{
    ffi::{c_char, c_void, CStr, CString},
    ptr::null,
};

pub const AIRPLAY_OUTPUT_STATE_EVENT: &str = "openkara://airplay-output-state";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum AirPlayAudienceMode {
    Idle,
    Lyrics,
    Cdg,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum AirPlayOutputPhase {
    Idle,
    RouteSelected,
    Buffering,
    Playing,
    Failed,
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

#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AirPlayColor {
    pub red: f64,
    pub green: f64,
    pub blue: f64,
    pub alpha: f64,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AudiencePresentationSpec {
    pub content_width_ratio: f64,
    pub content_max_width_px: u32,
    pub horizontal_padding_px: u32,
    pub vertical_padding_px: u32,
    pub line_gap_px: u32,
    pub font_size_px: u32,
    pub line_height_multiple: f64,
    pub active_scale: f64,
    pub status_font_size_px: u32,
    pub active_glow_blur_px: u32,
    pub active_text_color: AirPlayColor,
    pub past_text_color: AirPlayColor,
    pub future_text_color: AirPlayColor,
    pub plain_text_color: AirPlayColor,
    pub status_text_color: AirPlayColor,
    pub active_glow_color: AirPlayColor,
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
    pub lines: Vec<LyricLine>,
    pub offset_ms: i64,
    pub is_loading: bool,
    pub lyrics_font_step: i8,
    pub messages: AirPlayAudienceMessages,
    pub viewport: AirPlayViewport,
    pub presentation_spec: AudiencePresentationSpec,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum AirPlayPlainTextPageDirection {
    Prev,
    Next,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
struct AirPlayBridgeWordToken {
    time_ms: u64,
    text: String,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
struct AirPlayBridgeLyricLine {
    // RATIONALE: The native AirPlay bridge consumes a JSON scene with fixed
    // `timeMs` keys. Reusing the shared IPC lyric structs here would serialize
    // nested timestamps as `time_ms`, which silently downgrades timed lyrics to
    // plain text on TV. Keep this DTO separate even if it looks redundant.
    time_ms: u64,
    text: String,
    words: Option<Vec<AirPlayBridgeWordToken>>,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
struct AirPlayAudienceSceneConfigPayload {
    lyrics_song_id: Option<String>,
    lines: Vec<AirPlayBridgeLyricLine>,
    messages: AirPlayAudienceMessages,
    viewport: AirPlayViewport,
    presentation_spec: AudiencePresentationSpec,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
struct AirPlayAudienceRuntimePayload {
    mode: AirPlayAudienceMode,
    song_id: Option<String>,
    is_playing: bool,
    position_ms: u64,
    // RATIONALE: Native rendering must consume backend-derived runtime timing
    // directly. Re-deriving these values in Obj-C from loosely shaped scene
    // data already caused drift and incorrect "plain text" detection.
    adjusted_ms: i64,
    is_plain_text: bool,
    lyrics_match_current_song: bool,
    active_line_index: i64,
    is_loading: bool,
    stream_generation: u64,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct AirPlayOutputStateEvent {
    pub active: bool,
    pub audio_active: bool,
    pub route_name: Option<String>,
    pub mode: AirPlayAudienceMode,
    pub phase: AirPlayOutputPhase,
    pub detail: Option<String>,
    pub displayed_position_ms: Option<u64>,
    pub stream_generation: u64,
    pub latency_ms: Option<u64>,
}

#[derive(Default)]
struct AirPlayRuntimeState {
    app_handle: Option<AppHandle>,
    latest_payload: Option<AirPlayAudienceStatePayload>,
    local_audio_suppressed: Option<Arc<AtomicBool>>,
}

fn airplay_runtime_state() -> &'static Mutex<AirPlayRuntimeState> {
    static AIRPLAY_RUNTIME_STATE: OnceLock<Mutex<AirPlayRuntimeState>> = OnceLock::new();
    AIRPLAY_RUNTIME_STATE.get_or_init(|| Mutex::new(AirPlayRuntimeState::default()))
}

pub fn normalize_host_y(host_height: f64, dom_top: f64, host_height_for_view: f64) -> f64 {
    host_height - dom_top - host_height_for_view
}

#[cfg(target_os = "macos")]
fn mode_from_tag(tag: i32) -> AirPlayAudienceMode {
    match tag {
        1 => AirPlayAudienceMode::Lyrics,
        2 => AirPlayAudienceMode::Cdg,
        _ => AirPlayAudienceMode::Idle,
    }
}

#[cfg(target_os = "macos")]
fn phase_from_tag(tag: i32) -> AirPlayOutputPhase {
    match tag {
        1 => AirPlayOutputPhase::RouteSelected,
        2 => AirPlayOutputPhase::Buffering,
        3 => AirPlayOutputPhase::Playing,
        4 => AirPlayOutputPhase::Failed,
        _ => AirPlayOutputPhase::Idle,
    }
}

impl Default for AirPlayAudienceMessages {
    fn default() -> Self {
        Self {
            select_song: "Select a song to start".to_owned(),
            loading_lyrics: "Loading lyrics...".to_owned(),
            no_lyrics: "No lyrics available for this track".to_owned(),
            add_lyrics: "Add Lyrics".to_owned(),
        }
    }
}

impl Default for AirPlayViewport {
    fn default() -> Self {
        Self {
            width_px: 1280,
            height_px: 720,
            bottom_inset_px: 0,
        }
    }
}

impl Default for AudiencePresentationSpec {
    fn default() -> Self {
        Self {
            content_width_ratio: 0.92,
            content_max_width_px: 1_600,
            horizontal_padding_px: 64,
            vertical_padding_px: 56,
            line_gap_px: 40,
            font_size_px: 72,
            line_height_multiple: 1.08,
            active_scale: 1.05,
            status_font_size_px: 18,
            active_glow_blur_px: 12,
            active_text_color: AirPlayColor {
                red: 1.0,
                green: 1.0,
                blue: 1.0,
                alpha: 1.0,
            },
            past_text_color: AirPlayColor {
                red: 72.0 / 255.0,
                green: 72.0 / 255.0,
                blue: 74.0 / 255.0,
                alpha: 1.0,
            },
            future_text_color: AirPlayColor {
                red: 58.0 / 255.0,
                green: 58.0 / 255.0,
                blue: 60.0 / 255.0,
                alpha: 1.0,
            },
            plain_text_color: AirPlayColor {
                red: 1.0,
                green: 1.0,
                blue: 1.0,
                alpha: 1.0,
            },
            status_text_color: AirPlayColor {
                red: 142.0 / 255.0,
                green: 142.0 / 255.0,
                blue: 147.0 / 255.0,
                alpha: 1.0,
            },
            active_glow_color: AirPlayColor {
                red: 1.0,
                green: 1.0,
                blue: 1.0,
                alpha: 0.8,
            },
        }
    }
}

fn default_scene_config() -> AirPlayAudienceSceneConfigPayload {
    AirPlayAudienceSceneConfigPayload {
        lyrics_song_id: None,
        lines: Vec::new(),
        messages: AirPlayAudienceMessages::default(),
        viewport: AirPlayViewport::default(),
        presentation_spec: AudiencePresentationSpec::default(),
    }
}

fn default_runtime_payload() -> AirPlayAudienceRuntimePayload {
    AirPlayAudienceRuntimePayload {
        mode: AirPlayAudienceMode::Idle,
        song_id: None,
        is_playing: false,
        position_ms: 0,
        adjusted_ms: 0,
        is_plain_text: false,
        lyrics_match_current_song: false,
        active_line_index: -1,
        is_loading: false,
        stream_generation: 1,
    }
}

fn binary_search_line(lines: &[LyricLine], current_ms: i64) -> i64 {
    let mut lo = 0usize;
    let mut hi = lines.len();
    let mut result = -1i64;

    while lo < hi {
        let mid = lo + (hi - lo) / 2;
        if (lines[mid].time_ms as i64) <= current_ms {
            result = mid as i64;
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    result
}

fn bridge_lines(lines: &[LyricLine]) -> Vec<AirPlayBridgeLyricLine> {
    lines
        .iter()
        .map(|line| AirPlayBridgeLyricLine {
            time_ms: line.time_ms,
            text: line.text.clone(),
            words: line.words.as_ref().map(|words| {
                words
                    .iter()
                    .map(|word| AirPlayBridgeWordToken {
                        time_ms: word.time_ms,
                        text: word.text.clone(),
                    })
                    .collect()
            }),
        })
        .collect()
}

fn build_scene_config(
    config: Option<&AirPlayAudienceStatePayload>,
) -> AirPlayAudienceSceneConfigPayload {
    let Some(config) = config else {
        return default_scene_config();
    };

    match config.mode {
        AirPlayAudienceMode::Idle | AirPlayAudienceMode::Cdg => AirPlayAudienceSceneConfigPayload {
            messages: config.messages.clone(),
            viewport: config.viewport.clone(),
            presentation_spec: config.presentation_spec.clone(),
            ..default_scene_config()
        },
        AirPlayAudienceMode::Lyrics => AirPlayAudienceSceneConfigPayload {
            lyrics_song_id: config.song_id.clone(),
            lines: bridge_lines(&config.lines),
            messages: config.messages.clone(),
            viewport: config.viewport.clone(),
            presentation_spec: config.presentation_spec.clone(),
        },
    }
}

fn build_runtime_payload(
    config: Option<&AirPlayAudienceStatePayload>,
    snapshot: &PlaybackStateSnapshot,
    stream_generation: u64,
) -> AirPlayAudienceRuntimePayload {
    let Some(song_id) = snapshot.song_id.clone() else {
        return AirPlayAudienceRuntimePayload {
            stream_generation,
            ..default_runtime_payload()
        };
    };

    let Some(config) = config else {
        return AirPlayAudienceRuntimePayload {
            stream_generation,
            ..default_runtime_payload()
        };
    };

    match config.mode {
        AirPlayAudienceMode::Idle => AirPlayAudienceRuntimePayload {
            stream_generation,
            ..default_runtime_payload()
        },
        AirPlayAudienceMode::Cdg => AirPlayAudienceRuntimePayload {
            mode: AirPlayAudienceMode::Cdg,
            song_id: Some(song_id),
            is_playing: snapshot.is_playing,
            position_ms: snapshot.position_ms,
            adjusted_ms: snapshot.position_ms as i64,
            is_plain_text: false,
            lyrics_match_current_song: false,
            active_line_index: -1,
            is_loading: false,
            stream_generation,
        },
        AirPlayAudienceMode::Lyrics => {
            let lyrics_match_current_song = config.song_id.as_deref() == Some(song_id.as_str());
            let lines: &[crate::lyrics::parser::LyricLine] = if lyrics_match_current_song {
                &config.lines
            } else {
                &[]
            };
            let is_plain_text = !lines.is_empty() && lines.iter().all(|line| line.time_ms == 0);
            let adjusted_ms = snapshot.position_ms as i64 - config.offset_ms;
            let active_line_index = if lines.is_empty() || is_plain_text {
                -1
            } else {
                binary_search_line(lines, adjusted_ms)
            };

            AirPlayAudienceRuntimePayload {
                mode: AirPlayAudienceMode::Lyrics,
                song_id: Some(song_id),
                is_playing: snapshot.is_playing,
                position_ms: snapshot.position_ms,
                adjusted_ms,
                is_plain_text,
                lyrics_match_current_song,
                active_line_index,
                is_loading: config.is_loading || !lyrics_match_current_song,
                stream_generation,
            }
        }
    }
}

fn read_latest_airplay_payload() -> Option<AirPlayAudienceStatePayload> {
    airplay_runtime_state()
        .lock()
        .ok()
        .and_then(|state| state.latest_payload.clone())
}

fn build_current_runtime_payload(
    playback: &Arc<Mutex<PlaybackController>>,
    stream_generation: &Arc<AtomicU64>,
) -> Option<AirPlayAudienceRuntimePayload> {
    let snapshot = playback
        .lock()
        .ok()
        .map(|mut controller| controller.snapshot(monotonic_now_ms()))?;
    Some(build_runtime_payload(
        read_latest_airplay_payload().as_ref(),
        &snapshot,
        stream_generation.load(Ordering::SeqCst),
    ))
}

fn build_current_cdg_frame(
    cdg_state: &Arc<Mutex<Option<CdgPlaybackState>>>,
    runtime: &AirPlayAudienceRuntimePayload,
) -> Option<Vec<u8>> {
    if runtime.mode != AirPlayAudienceMode::Cdg || runtime.song_id.is_none() {
        return None;
    }

    let mut cdg_state = cdg_state.lock().ok()?;
    render_cdg_frame_bytes(&mut cdg_state, runtime.position_ms)
}

fn spawn_airplay_audience_coordinator(
    playback: Arc<Mutex<PlaybackController>>,
    cdg_state: Arc<Mutex<Option<CdgPlaybackState>>>,
    stream_generation: Arc<AtomicU64>,
) {
    thread::spawn(move || loop {
        // RATIONALE: AirPlay must advance from backend playback time, not the
        // occlusion-prone main-window JS loop. Otherwise macOS can throttle the
        // source window and regress TV lyrics/CDG back to slideshow cadence.
        thread::sleep(stream_tick_interval());

        let Some(runtime) = build_current_runtime_payload(&playback, &stream_generation) else {
            continue;
        };
        let cdg_frame = build_current_cdg_frame(&cdg_state, &runtime);
        let _ = native::sync_audience_runtime(&runtime, cdg_frame.as_deref());
    });
}

pub fn ensure_airplay_audience_coordinator_started(
    playback: Arc<Mutex<PlaybackController>>,
    cdg_state: Arc<Mutex<Option<CdgPlaybackState>>>,
    stream_generation: Arc<AtomicU64>,
) {
    static AIRPLAY_AUDIENCE_COORDINATOR_STARTED: OnceLock<()> = OnceLock::new();
    AIRPLAY_AUDIENCE_COORDINATOR_STARTED.get_or_init(|| {
        spawn_airplay_audience_coordinator(playback, cdg_state, stream_generation);
    });
}

fn remember_runtime_handles(app_handle: &AppHandle, local_audio_suppressed: &Arc<AtomicBool>) {
    if let Ok(mut state) = airplay_runtime_state().lock() {
        state.app_handle = Some(app_handle.clone());
        state.local_audio_suppressed = Some(Arc::clone(local_audio_suppressed));
    }
}

#[cfg(target_os = "macos")]
fn emit_airplay_state(
    active: bool,
    audio_active: bool,
    route_name: Option<String>,
    mode: AirPlayAudienceMode,
    phase: AirPlayOutputPhase,
    detail: Option<String>,
    displayed_position_ms: Option<u64>,
    stream_generation: u64,
    latency_ms: Option<u64>,
) {
    let (handle, local_audio_suppressed) = airplay_runtime_state()
        .lock()
        .ok()
        .map(|state| {
            (
                state.app_handle.clone(),
                state.local_audio_suppressed.clone(),
            )
        })
        .unwrap_or((None, None));

    if let Some(local_audio_suppressed) = local_audio_suppressed {
        // Local speaker suppression must follow actual remote audio routing,
        // not just whether the audience/video surface is active.
        local_audio_suppressed.store(audio_active, Ordering::SeqCst);
    }

    if let Some(handle) = handle {
        let _ = handle.emit(
            AIRPLAY_OUTPUT_STATE_EVENT,
            AirPlayOutputStateEvent {
                active,
                audio_active,
                route_name,
                mode,
                phase,
                detail,
                displayed_position_ms,
                stream_generation,
                latency_ms,
            },
        );
    }
}

fn airplay_audience_playlist_url(base_url: &str) -> String {
    format!("{base_url}/audience-video.m3u8")
}

fn ensure_stream_server(state: &AppState) -> CommandResult<(PathBuf, String)> {
    let root_dir = default_stream_root(&state.app_data_dir);
    let mut server = state
        .airplay_http_server
        .lock()
        .map_err(|_| internal_error("airplay http server lock was poisoned".to_owned()))?;

    if server.is_none() {
        *server =
            Some(AirPlayHttpServer::bind(&root_dir).map_err(|error| {
                internal_error(format!("failed to start airplay server: {error}"))
            })?);
    }

    let base_url = server
        .as_ref()
        .map(|server| server.base_url().to_owned())
        .ok_or_else(|| internal_error("airplay server handle was not initialized".to_owned()))?;

    Ok((root_dir, airplay_audience_playlist_url(&base_url)))
}

#[cfg(target_os = "macos")]
mod native {
    use super::*;
    use std::sync::OnceLock;

    unsafe extern "C" {
        fn ok_airplay_set_state_callback(
            callback: extern "C" fn(
                bool,
                bool,
                *const c_char,
                i32,
                i32,
                *const c_char,
                i64,
                u64,
                i64,
            ),
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
        fn ok_airplay_sync_audience_state(config_json: *const c_char);
        fn ok_airplay_sync_audience_runtime(
            scene_json: *const c_char,
            cdg_frame_ptr: *const u8,
            cdg_frame_len: usize,
        );
        fn ok_airplay_step_plain_text_page(direction: i32) -> bool;
    }

    extern "C" fn handle_airplay_state_callback(
        active: bool,
        audio_active: bool,
        route_name: *const c_char,
        mode: i32,
        phase: i32,
        detail: *const c_char,
        displayed_position_ms: i64,
        stream_generation: u64,
        latency_ms: i64,
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

        let detail = if detail.is_null() {
            None
        } else {
            // SAFETY: Objective-C bridge passes a valid UTF-8 string pointer or NULL.
            Some(
                unsafe { CStr::from_ptr(detail) }
                    .to_string_lossy()
                    .into_owned(),
            )
        };

        emit_airplay_state(
            active,
            audio_active,
            route_name,
            mode_from_tag(mode),
            phase_from_tag(phase),
            detail,
            (displayed_position_ms >= 0).then_some(displayed_position_ms as u64),
            stream_generation,
            (latency_ms >= 0).then_some(latency_ms as u64),
        );
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

    pub(super) fn sync_audience_config(
        payload: &AirPlayAudienceSceneConfigPayload,
    ) -> CommandResult<()> {
        ensure_callback_registered();

        let config_json = serde_json::to_string(payload).map_err(|error| {
            internal_error(format!("failed to serialize airplay scene config: {error}"))
        })?;
        let config_json = CString::new(config_json.as_bytes()).map_err(|error| {
            internal_error(format!("invalid airplay scene config json: {error}"))
        })?;

        unsafe {
            ok_airplay_sync_audience_state(config_json.as_ptr());
        }

        Ok(())
    }

    pub(super) fn sync_audience_runtime(
        payload: &AirPlayAudienceRuntimePayload,
        cdg_frame: Option<&[u8]>,
    ) -> CommandResult<()> {
        ensure_callback_registered();

        let scene_json = serde_json::to_string(payload).map_err(|error| {
            internal_error(format!("failed to serialize airplay runtime: {error}"))
        })?;
        let scene_json = CString::new(scene_json.as_bytes())
            .map_err(|error| internal_error(format!("invalid airplay runtime json: {error}")))?;
        let (cdg_frame_ptr, cdg_frame_len) = match cdg_frame {
            Some(frame) => (frame.as_ptr(), frame.len()),
            None => (null::<u8>(), 0),
        };

        unsafe {
            ok_airplay_sync_audience_runtime(scene_json.as_ptr(), cdg_frame_ptr, cdg_frame_len);
        }

        Ok(())
    }

    pub(super) fn step_plain_text_page(direction: i32) -> bool {
        unsafe { ok_airplay_step_plain_text_page(direction) }
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

    pub(super) fn sync_audience_config(
        _payload: &AirPlayAudienceSceneConfigPayload,
    ) -> CommandResult<()> {
        Ok(())
    }

    pub(super) fn sync_audience_runtime(
        _payload: &AirPlayAudienceRuntimePayload,
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
    remember_runtime_handles(&app_handle, &state.airplay_local_output_suppressed);
    ensure_airplay_audience_coordinator_started(
        state.playback.clone(),
        state.cdg_state.clone(),
        state.airplay_stream_generation.clone(),
    );
    let (stream_root, playlist_url) = ensure_stream_server(&state)?;
    native::sync_route_picker(&window, &stream_root, &playlist_url, bounds)
}

#[tauri::command]
pub fn sync_airplay_audience_state(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    payload: AirPlayAudienceStatePayload,
) -> CommandResult<()> {
    remember_runtime_handles(&app_handle, &state.airplay_local_output_suppressed);
    ensure_airplay_audience_coordinator_started(
        state.playback.clone(),
        state.cdg_state.clone(),
        state.airplay_stream_generation.clone(),
    );

    if let Ok(mut runtime_state) = airplay_runtime_state().lock() {
        let previous_mode = runtime_state
            .latest_payload
            .as_ref()
            .map(|previous| previous.mode);
        runtime_state.latest_payload = Some(payload.clone());
        state
            .airplay_audience_active
            .store(payload.mode != AirPlayAudienceMode::Idle, Ordering::SeqCst);
        if previous_mode != Some(payload.mode) {
            state
                .airplay_stream_generation
                .fetch_add(1, Ordering::SeqCst);
        }
    }

    let snapshot = state
        .playback
        .lock()
        .map_err(|_| internal_error("playback controller lock was poisoned".to_owned()))?
        .snapshot(monotonic_now_ms());
    let scene_config = build_scene_config(Some(&payload));
    let runtime = build_runtime_payload(
        Some(&payload),
        &snapshot,
        state.airplay_stream_generation.load(Ordering::SeqCst),
    );
    let cdg_frame = build_current_cdg_frame(&state.cdg_state, &runtime);

    native::sync_audience_config(&scene_config)?;
    native::sync_audience_runtime(&runtime, cdg_frame.as_deref())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::audio::playback::StemVolumes;
    use crate::commands::bootstrap;
    use crate::separator::model_cache::ModelCache;
    use crate::AppState;
    use std::collections::HashMap;
    use std::path::PathBuf;
    use std::sync::{atomic::AtomicBool, Mutex};

    fn snapshot(
        song_id: Option<&str>,
        position_ms: u64,
        is_playing: bool,
    ) -> PlaybackStateSnapshot {
        PlaybackStateSnapshot {
            song_id: song_id.map(str::to_owned),
            is_playing,
            position_ms,
            duration_ms: Some(240_000),
            volume: 1.0,
            stem_volumes: StemVolumes {
                vocals: 1.0,
                drums: 1.0,
                bass: 1.0,
                other: 1.0,
            },
            has_stems: false,
            stem_mode: None,
        }
    }

    fn plain_text_airplay_state() -> AppState {
        AppState {
            library: Arc::new(Mutex::new(None)),
            app_data_dir: PathBuf::from(env!("CARGO_MANIFEST_DIR"))
                .join("tests")
                .join("tmp"),
            model_path: PathBuf::from("model.bin"),
            playback: Arc::new(Mutex::new(PlaybackController::default())),
            cdg_state: Arc::new(Mutex::new(None)),
            airplay_audio_tap: Arc::new(crate::airplay_stream::AirPlayAudioTap::new(4)),
            airplay_stream_generation: Arc::new(AtomicU64::new(7)),
            airplay_audience_active: Arc::new(AtomicBool::new(true)),
            airplay_control_refresh_token: Arc::new(AtomicU64::new(0)),
            airplay_http_server: Arc::new(Mutex::new(None)),
            airplay_local_output_suppressed: Arc::new(AtomicBool::new(false)),
            playback_request_id: AtomicU64::new(0),
            audio_output_started: Arc::new(AtomicBool::new(true)),
            audio_output_start_lock: Arc::new(Mutex::new(())),
            model_bootstrap_status: Arc::new(Mutex::new(bootstrap::pending_status("model.bin"))),
            separation_statuses: Arc::new(Mutex::new(HashMap::new())),
            separator_model_cache: Arc::new(Mutex::new(ModelCache::default())),
            batch_running: Arc::new(AtomicBool::new(false)),
            batch_cancel: Arc::new(AtomicBool::new(false)),
        }
    }

    #[test]
    fn build_runtime_payload_derives_active_line_from_playback_position() {
        let runtime = build_runtime_payload(
            Some(&AirPlayAudienceStatePayload {
                mode: AirPlayAudienceMode::Lyrics,
                song_id: Some("song-1".to_owned()),
                lines: vec![
                    LyricLine {
                        time_ms: 1_000,
                        text: "line 1".to_owned(),
                        words: None,
                    },
                    LyricLine {
                        time_ms: 2_000,
                        text: "line 2".to_owned(),
                        words: None,
                    },
                ],
                offset_ms: 100,
                is_loading: false,
                lyrics_font_step: 1,
                messages: AirPlayAudienceMessages::default(),
                viewport: AirPlayViewport::default(),
                presentation_spec: AudiencePresentationSpec::default(),
            }),
            &snapshot(Some("song-1"), 2_150, true),
            3,
        );

        assert_eq!(runtime.mode, AirPlayAudienceMode::Lyrics);
        assert_eq!(runtime.active_line_index, 1);
        assert_eq!(runtime.position_ms, 2_150);
        assert!(runtime.is_playing);
    }

    #[test]
    fn scene_config_serializes_bridge_line_times_as_camel_case() {
        let config = build_scene_config(Some(&AirPlayAudienceStatePayload {
            mode: AirPlayAudienceMode::Lyrics,
            song_id: Some("song-1".to_owned()),
            lines: vec![LyricLine {
                time_ms: 1_000,
                text: "line 1".to_owned(),
                words: Some(vec![crate::lyrics::parser::WordToken {
                    time_ms: 1_050,
                    text: "line".to_owned(),
                }]),
            }],
            offset_ms: 100,
            is_loading: false,
            lyrics_font_step: 0,
            messages: AirPlayAudienceMessages::default(),
            viewport: AirPlayViewport::default(),
            presentation_spec: AudiencePresentationSpec::default(),
        }));

        let json = serde_json::to_value(config).expect("config should serialize");
        assert_eq!(json["lyricsSongId"], "song-1");
        assert_eq!(json["lines"][0]["timeMs"], 1_000);
        assert_eq!(json["lines"][0]["time_ms"], serde_json::Value::Null);
        assert_eq!(json["lines"][0]["words"][0]["timeMs"], 1_050);
        assert_eq!(
            json["lines"][0]["words"][0]["time_ms"],
            serde_json::Value::Null
        );
    }

    #[test]
    fn runtime_payload_serializes_runtime_fields_without_lyrics_blob() {
        let runtime = build_runtime_payload(
            Some(&AirPlayAudienceStatePayload {
                mode: AirPlayAudienceMode::Lyrics,
                song_id: Some("song-1".to_owned()),
                lines: vec![LyricLine {
                    time_ms: 1_000,
                    text: "line 1".to_owned(),
                    words: Some(vec![crate::lyrics::parser::WordToken {
                        time_ms: 1_050,
                        text: "line".to_owned(),
                    }]),
                }],
                offset_ms: 100,
                is_loading: false,
                lyrics_font_step: 0,
                messages: AirPlayAudienceMessages::default(),
                viewport: AirPlayViewport::default(),
                presentation_spec: AudiencePresentationSpec::default(),
            }),
            &snapshot(Some("song-1"), 1_150, true),
            4,
        );

        let json = serde_json::to_value(runtime).expect("runtime should serialize");
        assert_eq!(json["adjustedMs"], 1_050);
        assert_eq!(json["isPlainText"], false);
        assert_eq!(json["lyricsMatchCurrentSong"], true);
        assert_eq!(json["streamGeneration"], 4);
        assert_eq!(json["lines"], serde_json::Value::Null);
    }

    #[test]
    fn build_runtime_payload_ignores_stale_lyrics_payloads() {
        let runtime = build_runtime_payload(
            Some(&AirPlayAudienceStatePayload {
                mode: AirPlayAudienceMode::Lyrics,
                song_id: Some("old-song".to_owned()),
                lines: vec![LyricLine {
                    time_ms: 1_000,
                    text: "stale".to_owned(),
                    words: None,
                }],
                offset_ms: 0,
                is_loading: false,
                lyrics_font_step: 0,
                messages: AirPlayAudienceMessages::default(),
                viewport: AirPlayViewport::default(),
                presentation_spec: AudiencePresentationSpec::default(),
            }),
            &snapshot(Some("new-song"), 1_500, true),
            5,
        );

        assert_eq!(runtime.song_id.as_deref(), Some("new-song"));
        assert!(!runtime.lyrics_match_current_song);
        assert!(runtime.is_loading);
        assert_eq!(runtime.active_line_index, -1);
    }

    #[test]
    fn build_runtime_payload_keeps_idle_output_blank() {
        let runtime = build_runtime_payload(
            Some(&AirPlayAudienceStatePayload {
                mode: AirPlayAudienceMode::Idle,
                song_id: Some("song-1".to_owned()),
                lines: vec![],
                offset_ms: 0,
                is_loading: false,
                lyrics_font_step: 0,
                messages: AirPlayAudienceMessages::default(),
                viewport: AirPlayViewport::default(),
                presentation_spec: AudiencePresentationSpec::default(),
            }),
            &snapshot(Some("song-1"), 4_200, true),
            6,
        );

        assert_eq!(runtime.mode, AirPlayAudienceMode::Idle);
        assert!(runtime.song_id.is_none());
        assert!(!runtime.is_playing);
    }

    #[test]
    fn plain_text_page_step_does_not_refresh_airplay_stream_when_audience_is_active() {
        let state = plain_text_airplay_state();
        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);
        let initial_epoch = state.airplay_audio_tap.current_epoch();

        let generation = refresh_airplay_stream_after_plain_text_page_step(&state);

        assert_eq!(generation, None);
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation
        );
        assert_eq!(state.airplay_audio_tap.current_epoch(), initial_epoch);
    }

    #[test]
    fn plain_text_page_step_does_not_refresh_airplay_stream_when_audience_is_idle() {
        let state = plain_text_airplay_state();
        state.airplay_audience_active.store(false, Ordering::SeqCst);
        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);
        let initial_epoch = state.airplay_audio_tap.current_epoch();

        let generation = refresh_airplay_stream_after_plain_text_page_step(&state);

        assert_eq!(generation, None);
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation
        );
        assert_eq!(state.airplay_audio_tap.current_epoch(), initial_epoch);
    }

    #[test]
    fn airplay_output_state_event_serializes_audio_active() {
        let value = serde_json::to_value(AirPlayOutputStateEvent {
            active: false,
            audio_active: true,
            route_name: None,
            mode: AirPlayAudienceMode::Lyrics,
            phase: AirPlayOutputPhase::Playing,
            detail: None,
            displayed_position_ms: None,
            stream_generation: 9,
            latency_ms: None,
        })
        .expect("event should serialize");

        assert_eq!(value["active"], false);
        assert_eq!(value["audioActive"], true);
        assert_eq!(value["streamGeneration"], 9);
    }

    #[test]
    fn airplay_route_picker_uses_audience_video_playlist() {
        assert_eq!(
            airplay_audience_playlist_url("http://192.168.1.8:8080"),
            "http://192.168.1.8:8080/audience-video.m3u8"
        );
    }
}

#[cfg(target_os = "macos")]
fn native_step_plain_text_page(direction: i32) -> bool {
    native::step_plain_text_page(direction)
}

#[cfg(not(target_os = "macos"))]
fn native_step_plain_text_page(_direction: i32) -> bool {
    false
}

fn refresh_airplay_stream_after_plain_text_page_step(_state: &AppState) -> Option<u64> {
    None
}

#[tauri::command]
pub fn step_airplay_plain_text_page(
    state: State<'_, AppState>,
    direction: AirPlayPlainTextPageDirection,
) -> CommandResult<()> {
    let scene = airplay_runtime_state()
        .lock()
        .ok()
        .and_then(|runtime| runtime.latest_payload.clone());

    let Some(scene) = scene else {
        return Ok(());
    };

    if scene.mode != AirPlayAudienceMode::Lyrics
        || scene.lines.is_empty()
        || !scene.lines.iter().all(|line| line.time_ms == 0)
    {
        return Ok(());
    }

    let direction = match direction {
        AirPlayPlainTextPageDirection::Prev => -1,
        AirPlayPlainTextPageDirection::Next => 1,
    };
    if !native_step_plain_text_page(direction) {
        return Ok(());
    }

    let _ = refresh_airplay_stream_after_plain_text_page_step(&state);

    let snapshot = state
        .playback
        .lock()
        .map_err(|_| internal_error("playback controller lock was poisoned".to_owned()))?
        .snapshot(monotonic_now_ms());
    let runtime = build_runtime_payload(
        Some(&scene),
        &snapshot,
        state.airplay_stream_generation.load(Ordering::SeqCst),
    );
    let cdg_frame = build_current_cdg_frame(&state.cdg_state, &runtime);
    native::sync_audience_runtime(&runtime, cdg_frame.as_deref())?;

    Ok(())
}
