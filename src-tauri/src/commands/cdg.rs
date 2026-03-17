use crate::{
    cdg::{CdgPacket, CdgRenderer},
    commands::error::{internal_error, CommandResult},
    AppState,
};
use base64::{engine::general_purpose::STANDARD as BASE64, Engine};
use tauri::State;

/// Holds the CDG rendering state for the currently playing CD+G song.
pub struct CdgPlaybackState {
    pub packets: Vec<CdgPacket>,
    pub renderer: CdgRenderer,
    /// Last packet index that was processed.
    pub last_packet_index: usize,
    /// Cached RGBA frame as base64 string.
    pub cached_frame: Option<String>,
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

/// Returns a base64-encoded RGBA frame (288x192) for the given playback
/// position, or `null` if no CDG is active or the frame hasn't changed.
#[tauri::command]
pub fn get_cdg_frame(
    state: State<'_, AppState>,
    position_ms: u64,
) -> CommandResult<Option<String>> {
    let mut cdg_guard = state
        .cdg_state
        .lock()
        .map_err(|_| internal_error("CDG state lock was poisoned".to_owned()))?;

    let cdg = match cdg_guard.as_mut() {
        Some(cdg) => cdg,
        None => return Ok(None),
    };

    // 300 packets per second → packet_index = position_ms * 300 / 1000
    let target_index = ((position_ms as u128 * 300) / 1000) as usize;
    let target_index = target_index.min(cdg.packets.len());

    // Handle backward seek or reset
    if target_index < cdg.last_packet_index || cdg.needs_reset {
        cdg.seek_to(target_index);
        cdg.needs_reset = false;
        let rgba = cdg.renderer.to_rgba();
        let encoded = BASE64.encode(&rgba);
        cdg.cached_frame = Some(encoded.clone());
        return Ok(Some(encoded));
    }

    // Forward: process from last position to target
    if target_index > cdg.last_packet_index {
        let changed = cdg
            .renderer
            .process_range(&cdg.packets, cdg.last_packet_index, target_index);
        cdg.last_packet_index = target_index;

        if changed || cdg.cached_frame.is_none() {
            let rgba = cdg.renderer.to_rgba();
            let encoded = BASE64.encode(&rgba);
            cdg.cached_frame = Some(encoded.clone());
            return Ok(Some(encoded));
        }
    }

    // No change
    Ok(None)
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
