use crate::{
    cdg::{parse_cdg_bytes, parse_cdg_file},
    commands::cdg::CdgPlaybackState,
    library_root::LibraryRoot,
    media_g::{self, MEDIA_G_ZIP},
};
use std::path::Path;

pub fn load_cdg_state_for_song(
    library_root: &LibraryRoot,
    song: &crate::library::Song,
) -> Option<CdgPlaybackState> {
    let absolute_path = library_root.resolve(&song.file_path);
    match song.media_g_container.as_deref() {
        Some(MEDIA_G_ZIP) => load_cdg_state_from_zip(&absolute_path),
        _ => {
            if let Some(cdg_path) = song.cdg_path.as_deref() {
                return load_cdg_state_from_explicit_path(&library_root.resolve(cdg_path));
            }
            load_cdg_sidecar_state(&absolute_path)
        }
    }
}

pub fn mark_cdg_reset_for_seek(
    cdg_state: &mut Option<CdgPlaybackState>,
    previous_ms: u64,
    next_ms: u64,
) {
    if next_ms < previous_ms {
        if let Some(cdg_state) = cdg_state.as_mut() {
            // CDG rendering is stateful, so backward seeks must rebuild from the
            // start instead of trying to "rewind" incremental packet application.
            cdg_state.needs_reset = true;
            cdg_state.cached_frame = None;
        }
    }
}

fn load_cdg_sidecar_state(audio_path: &Path) -> Option<CdgPlaybackState> {
    let sidecar_path = audio_path.with_extension("cdg");
    if !sidecar_path.is_file() {
        return None;
    }

    match parse_cdg_file(&sidecar_path) {
        Ok(packets) => Some(CdgPlaybackState::new(packets)),
        Err(error) => {
            // CDG graphics are optional sidecars. A broken `.cdg` file should
            // not prevent the audio track itself from starting playback.
            eprintln!(
                "warning: failed to parse CDG sidecar at {}: {}",
                sidecar_path.display(),
                error
            );
            None
        }
    }
}

fn load_cdg_state_from_explicit_path(cdg_path: &Path) -> Option<CdgPlaybackState> {
    if !cdg_path.is_file() {
        return None;
    }

    match parse_cdg_file(cdg_path) {
        Ok(packets) => Some(CdgPlaybackState::new(packets)),
        Err(error) => {
            eprintln!(
                "warning: failed to parse CDG sidecar at {}: {}",
                cdg_path.display(),
                error
            );
            None
        }
    }
}

fn load_cdg_state_from_zip(zip_path: &Path) -> Option<CdgPlaybackState> {
    match media_g::inspect_zip_for_media_g(zip_path) {
        Ok(asset) => Some(CdgPlaybackState::new(parse_cdg_bytes(&asset.cdg_bytes))),
        Err(error) => {
            eprintln!(
                "warning: failed to read CDG packets from Media+G ZIP at {}: {}",
                zip_path.display(),
                error
            );
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn loads_same_basename_cdg_sidecar_when_present() {
        let dir = tempfile::tempdir().expect("temp dir should be created");
        let audio_path = dir.path().join("track.mp3");
        std::fs::write(&audio_path, b"audio").expect("audio fixture should be written");

        let mut packet = [0u8; 24];
        packet[0] = 0x09;
        packet[1] = 0x01;
        packet[4] = 0x07;
        std::fs::write(audio_path.with_extension("cdg"), packet)
            .expect("cdg fixture should be written");

        let state = load_cdg_sidecar_state(&audio_path).expect("expected CDG state");
        assert_eq!(state.packets.len(), 1);
        assert_eq!(state.last_packet_index, 0);
        assert!(state.needs_reset);
    }

    #[test]
    fn truncated_cdg_sidecar_yields_empty_state_without_failing() {
        let dir = tempfile::tempdir().expect("temp dir should be created");
        let audio_path = dir.path().join("track.mp3");
        std::fs::write(&audio_path, b"audio").expect("audio fixture should be written");
        std::fs::write(audio_path.with_extension("cdg"), [0x09, 0x01, 0x07])
            .expect("broken cdg fixture should be written");

        let state = load_cdg_sidecar_state(&audio_path)
            .expect("existing sidecar should still produce a playback state");
        assert!(state.packets.is_empty());
    }

    #[test]
    fn backward_seek_marks_cdg_state_for_reset() {
        let packets = vec![crate::cdg::parser::CdgPacket {
            command: 0x09,
            instruction: 0x01,
            data: [0; 16],
        }];
        let mut cdg_state = Some(CdgPlaybackState::new(packets));

        if let Some(cdg_state) = cdg_state.as_mut() {
            cdg_state.needs_reset = false;
            cdg_state.cached_frame = Some(vec![0xAA, 0xBB, 0xCC]);
        }

        mark_cdg_reset_for_seek(&mut cdg_state, 2_000, 1_000);

        let cdg_state = cdg_state.expect("CDG state should still exist");
        assert!(cdg_state.needs_reset);
        assert!(cdg_state.cached_frame.is_none());
    }

    #[test]
    fn forward_seek_keeps_existing_cdg_state() {
        let packets = vec![crate::cdg::parser::CdgPacket {
            command: 0x09,
            instruction: 0x01,
            data: [0; 16],
        }];
        let mut cdg_state = Some(CdgPlaybackState::new(packets));

        if let Some(cdg_state) = cdg_state.as_mut() {
            cdg_state.needs_reset = false;
            cdg_state.cached_frame = Some(vec![0xAA, 0xBB, 0xCC]);
        }

        mark_cdg_reset_for_seek(&mut cdg_state, 1_000, 2_000);

        let cdg_state = cdg_state.expect("CDG state should still exist");
        assert!(!cdg_state.needs_reset);
        assert_eq!(
            cdg_state.cached_frame.as_deref(),
            Some([0xAA, 0xBB, 0xCC].as_slice())
        );
    }
}
