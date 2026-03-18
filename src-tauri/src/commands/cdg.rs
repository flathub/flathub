use crate::{
    cdg::{CdgPacket, CdgRenderer},
    commands::error::{internal_error, CommandResult},
    AppState,
};
use tauri::{ipc::Response, State};

/// Holds the CDG rendering state for the currently playing CD+G song.
pub struct CdgPlaybackState {
    pub packets: Vec<CdgPacket>,
    pub renderer: CdgRenderer,
    /// Last packet index that was processed.
    pub last_packet_index: usize,
    /// Cached raw RGBA frame bytes (288×192×4 = 221,184 bytes).
    ///
    /// PERF: Stored as raw `Vec<u8>` instead of base64 `String` to avoid the
    /// ~33% inflation and CPU cost of base64 encoding on every frame. The IPC
    /// layer returns these bytes directly as an `ArrayBuffer` via
    /// `tauri::ipc::Response`, so the frontend can wrap them in a
    /// `Uint8ClampedArray` at O(1) cost instead of running an O(n) decode loop.
    /// Do not revert to base64 — it adds measurable overhead at 30fps.
    pub cached_frame: Option<Vec<u8>>,
    /// Whether the renderer needs a full reset (e.g. after backward seek).
    pub needs_reset: bool,
    /// Monotonically increasing version counter. Incremented each time
    /// `cached_frame` changes. The fullscreen window sends its last-known
    /// version so `get_cdg_display_frame` can skip the 221KB clone when
    /// the frame hasn't changed.
    pub frame_version: u64,
}

impl CdgPlaybackState {
    pub fn new(packets: Vec<CdgPacket>) -> Self {
        Self {
            packets,
            renderer: CdgRenderer::new(),
            last_packet_index: 0,
            cached_frame: None,
            // Start dirty so the first poll at 0ms produces an actual frame.
            // Otherwise the frontend cannot distinguish "active CDG, initial frame"
            // from "no new frame, keep whatever was on screen before".
            needs_reset: true,
            frame_version: 0,
        }
    }

    /// Reset the renderer and re-render up to the given packet index.
    pub fn seek_to(&mut self, packet_index: usize) {
        self.renderer
            .reset_and_render_to(&self.packets, packet_index);
        self.last_packet_index = packet_index;
        // Force a fresh frame on next get_cdg_frame call.
        self.cached_frame = None;
    }
}

/// Returns a raw RGBA frame (288×192, 221,184 bytes) as binary `ArrayBuffer`
/// for the given playback position.
///
/// Returns an **empty body** (0 bytes) when no CDG is active or when the
/// visual frame hasn't changed since the last call. The frontend distinguishes
/// "new frame" from "no change" by checking `ArrayBuffer.byteLength > 0`.
///
/// PERF: Uses `tauri::ipc::Response` to return raw bytes directly, bypassing
/// JSON serialization and base64 encoding. This eliminates ~33% IPC payload
/// inflation and the expensive `atob` + charCodeAt decode loop on the JS side.
/// Combined with a pre-allocated `ImageData` on the frontend, CDG rendering
/// adds near-zero overhead to the main thread's frame budget. Do not change
/// this to return a JSON-serialized type (e.g. `Option<String>` with base64)
/// without benchmarking — the previous base64 path was the main CDG bottleneck.
#[tauri::command]
pub fn get_cdg_frame(
    state: State<'_, AppState>,
    position_ms: u64,
) -> CommandResult<Response> {
    let mut cdg_guard = state
        .cdg_state
        .lock()
        .map_err(|_| internal_error("CDG state lock was poisoned".to_owned()))?;

    let cdg = match cdg_guard.as_mut() {
        Some(cdg) => cdg,
        None => return Ok(Response::new(Vec::<u8>::new())),
    };

    // 300 packets per second → packet_index = position_ms * 300 / 1000
    let target_index = ((position_ms as u128 * 300) / 1000) as usize;
    let target_index = target_index.min(cdg.packets.len());

    // Handle backward seek or reset
    if target_index < cdg.last_packet_index || cdg.needs_reset {
        cdg.seek_to(target_index);
        cdg.needs_reset = false;
        let rgba = cdg.renderer.to_rgba();
        cdg.cached_frame = Some(rgba.clone());
        cdg.frame_version += 1;
        return Ok(Response::new(rgba));
    }

    // Forward: process from last position to target
    if target_index > cdg.last_packet_index {
        let changed = cdg
            .renderer
            .process_range(&cdg.packets, cdg.last_packet_index, target_index);
        cdg.last_packet_index = target_index;

        if changed || cdg.cached_frame.is_none() {
            let rgba = cdg.renderer.to_rgba();
            cdg.cached_frame = Some(rgba.clone());
            cdg.frame_version += 1;
            return Ok(Response::new(rgba));
        }
    }

    // No change — empty body signals "keep current frame"
    Ok(Response::new(Vec::<u8>::new()))
}

/// Returns the last rendered CDG frame without advancing the renderer.
///
/// Used by the fullscreen window to mirror the main window's CDG display.
/// The main window drives the renderer via `get_cdg_frame`; this command
/// simply returns its `cached_frame`.
///
/// The response is a binary blob: **8-byte little-endian `u64` version**
/// followed by the RGBA frame bytes (221,184 bytes). When the caller's
/// `last_version` matches the current version, only the 8-byte version
/// header is returned (no frame data) — signaling "no change". An empty
/// body (0 bytes) means no CDG is active.
#[tauri::command]
pub fn get_cdg_display_frame(
    state: State<'_, AppState>,
    last_version: u64,
) -> CommandResult<Response> {
    let cdg_guard = state
        .cdg_state
        .lock()
        .map_err(|_| internal_error("CDG state lock was poisoned".to_owned()))?;

    let cdg = match cdg_guard.as_ref() {
        Some(cdg) => cdg,
        None => return Ok(Response::new(Vec::<u8>::new())),
    };

    let version = cdg.frame_version;

    // No change since last poll — return version header only (8 bytes).
    if version == last_version {
        return Ok(Response::new(version.to_le_bytes().to_vec()));
    }

    match cdg.cached_frame.as_ref() {
        Some(frame) => {
            let mut buf = Vec::with_capacity(8 + frame.len());
            buf.extend_from_slice(&version.to_le_bytes());
            buf.extend_from_slice(frame);
            Ok(Response::new(buf))
        }
        None => Ok(Response::new(Vec::<u8>::new())),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_cdg_state_requests_initial_frame_render() {
        let state = CdgPlaybackState::new(Vec::new());

        assert!(state.needs_reset);
        assert!(state.cached_frame.is_none());
        assert_eq!(state.last_packet_index, 0);
    }
}
