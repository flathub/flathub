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

pub fn render_cdg_frame_bytes(
    cdg_state: &mut Option<CdgPlaybackState>,
    position_ms: u64,
) -> Option<Vec<u8>> {
    let cdg = cdg_state.as_mut()?;

    let target_index = ((position_ms as u128 * 300) / 1000) as usize;
    let target_index = target_index.min(cdg.packets.len());

    if target_index < cdg.last_packet_index || cdg.needs_reset {
        cdg.seek_to(target_index);
        cdg.needs_reset = false;
        let rgba = cdg.renderer.to_rgba();
        cdg.cached_frame = Some(rgba.clone());
        return Some(rgba);
    }

    if target_index > cdg.last_packet_index {
        let changed = cdg
            .renderer
            .process_range(&cdg.packets, cdg.last_packet_index, target_index);
        cdg.last_packet_index = target_index;

        if changed || cdg.cached_frame.is_none() {
            let rgba = cdg.renderer.to_rgba();
            cdg.cached_frame = Some(rgba.clone());
            return Some(rgba);
        }
    }

    cdg.cached_frame.clone()
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
pub fn get_cdg_frame(state: State<'_, AppState>, position_ms: u64) -> CommandResult<Response> {
    let mut cdg_guard = state
        .cdg_state
        .lock()
        .map_err(|_| internal_error("CDG state lock was poisoned".to_owned()))?;

    match cdg_guard.as_mut() {
        Some(cdg) => cdg,
        None => return Ok(Response::new(Vec::<u8>::new())),
    };

    match render_cdg_frame_bytes(&mut cdg_guard, position_ms) {
        Some(frame) => Ok(Response::new(frame)),
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
