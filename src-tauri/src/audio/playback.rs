use crate::audio::decode::DecodedAudio;
use anyhow::{Result, bail};
use serde::{Deserialize, Serialize};
use std::sync::OnceLock;
use std::time::Instant;

pub const PLAYBACK_POSITION_EVENT: &str = "playback-position";
pub const PLAYBACK_ENDED_EVENT: &str = "playback-ended";
pub const PLAYBACK_POSITION_POLL_INTERVAL_MS: u64 = 33;

#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct StemVolumes {
    pub vocals: f32,
    pub drums: f32,
    pub bass: f32,
    pub other: f32,
}

impl Default for StemVolumes {
    fn default() -> Self {
        Self {
            vocals: 1.0,
            drums: 1.0,
            bass: 1.0,
            other: 1.0,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum StemName {
    Vocals,
    Drums,
    Bass,
    Other,
}

#[derive(Debug)]
pub struct StemSet {
    pub vocals: DecodedAudio,
    pub drums: DecodedAudio,
    pub bass: DecodedAudio,
    pub other: DecodedAudio,
}

#[derive(Debug)]
pub enum LoadedStems {
    /// Vocals + mixed accompaniment (2-stem mode)
    TwoStem {
        vocals: DecodedAudio,
        accompaniment: DecodedAudio,
    },
    /// Individual stems (4-stem mode)
    FourStem(StemSet),
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct PlaybackStateSnapshot {
    pub song_id: Option<String>,
    /// Backend transport lifecycle; pause is represented by `is_playing: false`.
    /// `playing` means a decoded track owns the transport, not that time is advancing.
    pub state: String,
    pub is_playing: bool,
    pub position_ms: u64,
    pub duration_ms: Option<u64>,
    pub volume: f32,
    pub stem_volumes: StemVolumes,
    pub has_stems: bool,
    pub stem_mode: Option<String>,
}

impl PlaybackStateSnapshot {
    pub fn idle() -> Self {
        Self {
            song_id: None,
            state: "idle".to_owned(),
            is_playing: false,
            position_ms: 0,
            duration_ms: None,
            volume: 1.0,
            stem_volumes: StemVolumes::default(),
            has_stems: false,
            stem_mode: None,
        }
    }
}

#[derive(Debug)]
pub(crate) struct LoadedTrack {
    pub(crate) song_id: String,
    pub(crate) original_audio: DecodedAudio,
    pub(crate) stems: Option<LoadedStems>,
    base_position_ms: u64,
    started_at_ms: Option<u64>,
    /// Source-rate frame index where the audio output thread renders from next.
    /// Updated exclusively by the render callback; reset by seek/start_track.
    pub(crate) render_frame: u64,
}

#[derive(Debug)]
pub struct PlaybackController {
    pub(crate) current_track: Option<LoadedTrack>,
    loading_song_id: Option<String>,
    volume: f32,
    stem_volumes: StemVolumes,
}

impl Default for PlaybackController {
    fn default() -> Self {
        Self {
            current_track: None,
            loading_song_id: None,
            volume: 1.0,
            stem_volumes: StemVolumes::default(),
        }
    }
}

impl PlaybackController {
    pub fn start_track(
        &mut self,
        song_id: String,
        decoded_audio: DecodedAudio,
        now_ms: u64,
    ) -> PlaybackStateSnapshot {
        self.loading_song_id = None;
        self.current_track = Some(LoadedTrack {
            song_id,
            original_audio: decoded_audio,
            stems: None,
            base_position_ms: 0,
            started_at_ms: Some(now_ms),
            render_frame: 0,
        });
        self.snapshot(now_ms)
    }

    /// Mark a track as loading — the audio data will arrive later from a
    /// background download/decode task.  The snapshot reports `state: "loading"`
    /// so the UI can show a spinner without freezing the window.
    pub fn start_track_loading(&mut self, song_id: &str) -> PlaybackStateSnapshot {
        self.current_track = None;
        self.loading_song_id = Some(song_id.to_owned());
        self.snapshot(monotonic_now_ms())
    }

    pub fn play(&mut self, now_ms: u64) -> Result<PlaybackStateSnapshot> {
        let track = self
            .current_track
            .as_mut()
            .ok_or_else(|| anyhow::anyhow!("no track is loaded"))?;
        let position_ms = track.position_ms(now_ms);
        track.base_position_ms = position_ms;
        track.started_at_ms = Some(now_ms);

        Ok(self.snapshot(now_ms))
    }

    pub fn pause(&mut self, now_ms: u64) -> Result<PlaybackStateSnapshot> {
        let track = self
            .current_track
            .as_mut()
            .ok_or_else(|| anyhow::anyhow!("no track is loaded"))?;
        let position_ms = track.position_ms(now_ms);
        track.base_position_ms = position_ms;
        track.started_at_ms = None;

        Ok(self.snapshot(now_ms))
    }

    pub fn seek(&mut self, target_ms: u64, now_ms: u64) -> Result<PlaybackStateSnapshot> {
        let track = self
            .current_track
            .as_mut()
            .ok_or_else(|| anyhow::anyhow!("no track is loaded"))?;
        let clamped_ms = target_ms.min(track.duration_ms());
        track.base_position_ms = clamped_ms;
        if track.started_at_ms.is_some() {
            track.started_at_ms = Some(now_ms);
        }
        // Reset render frame to match the new seek position
        let sample_rate = track.original_audio.sample_rate as f64;
        track.render_frame = (clamped_ms as f64 * sample_rate / 1000.0) as u64;

        Ok(self.snapshot(now_ms))
    }

    pub fn set_volume(&mut self, level: f32) -> Result<PlaybackStateSnapshot> {
        self.volume = level.clamp(0.0, 1.0);
        Ok(self.snapshot(monotonic_now_ms()))
    }

    pub fn set_stem_volume(&mut self, stem: StemName, level: f32) -> Result<PlaybackStateSnapshot> {
        let level = level.clamp(0.0, 1.0);
        match stem {
            StemName::Vocals => self.stem_volumes.vocals = level,
            StemName::Drums => self.stem_volumes.drums = level,
            StemName::Bass => self.stem_volumes.bass = level,
            StemName::Other => self.stem_volumes.other = level,
        }
        Ok(self.snapshot(monotonic_now_ms()))
    }

    pub fn attach_stems(&mut self, song_id: &str, stems: LoadedStems) -> Result<()> {
        let track = self
            .current_track
            .as_mut()
            .ok_or_else(|| anyhow::anyhow!("no track is loaded"))?;
        if track.song_id != song_id {
            bail!(
                "cannot attach stems for song {} while {} is loaded",
                song_id,
                track.song_id
            );
        }
        track.stems = Some(stems);
        Ok(())
    }

    pub fn has_stems(&self) -> bool {
        self.current_track
            .as_ref()
            .and_then(|t| t.stems.as_ref())
            .is_some()
    }

    /// Returns the stem mode string if stems are loaded: "two_stem" or "four_stem".
    pub fn stem_variant(&self) -> Option<&str> {
        self.current_track
            .as_ref()
            .and_then(|t| t.stems.as_ref())
            .map(|s| match s {
                LoadedStems::TwoStem { .. } => "two_stem",
                LoadedStems::FourStem(_) => "four_stem",
            })
    }

    pub fn snapshot(&mut self, now_ms: u64) -> PlaybackStateSnapshot {
        if let Some(track) = self.current_track.as_mut() {
            let raw_position = track.position_ms(now_ms);
            let duration_ms = track.duration_ms();

            // Clamp to duration and stop playback if past the end.
            let position_ms = if raw_position >= duration_ms {
                track.base_position_ms = duration_ms;
                track.started_at_ms = None;
                duration_ms
            } else {
                raw_position
            };

            let stem_mode = track.stems.as_ref().map(|s| match s {
                LoadedStems::TwoStem { .. } => "two_stem".to_owned(),
                LoadedStems::FourStem(_) => "four_stem".to_owned(),
            });

            return PlaybackStateSnapshot {
                song_id: Some(track.song_id.clone()),
                state: "playing".to_owned(),
                is_playing: track.started_at_ms.is_some(),
                position_ms,
                duration_ms: Some(duration_ms),
                volume: self.volume,
                stem_volumes: self.stem_volumes,
                has_stems: track.stems.is_some(),
                stem_mode,
            };
        }

        if let Some(song_id) = &self.loading_song_id {
            return PlaybackStateSnapshot {
                song_id: Some(song_id.clone()),
                state: "loading".to_owned(),
                is_playing: false,
                position_ms: 0,
                duration_ms: None,
                volume: self.volume,
                stem_volumes: self.stem_volumes,
                has_stems: false,
                stem_mode: None,
            };
        }

        self.idle_snapshot()
    }

    pub fn current_song_id(&self) -> Option<&str> {
        self.current_track
            .as_ref()
            .map(|track| track.song_id.as_str())
    }

    pub fn clear_track(&mut self) {
        self.current_track = None;
        self.loading_song_id = None;
    }

    fn idle_snapshot(&self) -> PlaybackStateSnapshot {
        PlaybackStateSnapshot {
            volume: self.volume,
            stem_volumes: self.stem_volumes,
            ..PlaybackStateSnapshot::idle()
        }
    }

    /// Returns the current render frame (source-rate frame index).
    pub fn current_render_frame(&self) -> u64 {
        self.current_track.as_ref().map_or(0, |t| t.render_frame)
    }

    /// Advance the render frame counter after the output callback renders audio.
    pub fn advance_render_frame(&mut self, frames: u64) {
        if let Some(track) = &mut self.current_track {
            track.render_frame += frames;
        }
    }
}

impl LoadedTrack {
    fn duration_ms(&self) -> u64 {
        self.original_audio.duration_ms
    }

    fn position_ms(&self, now_ms: u64) -> u64 {
        let elapsed_ms = self
            .started_at_ms
            .map(|started_at_ms| now_ms.saturating_sub(started_at_ms))
            .unwrap_or(0);

        (self.base_position_ms + elapsed_ms).min(self.duration_ms())
    }
}

pub fn monotonic_now_ms() -> u64 {
    static START: OnceLock<Instant> = OnceLock::new();
    START
        .get_or_init(Instant::now)
        .elapsed()
        .as_millis()
        .try_into()
        .unwrap_or(u64::MAX)
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct PlaybackPositionEvent {
    pub ms: u64,
    pub snapshot: PlaybackStateSnapshot,
}

#[derive(Debug, Clone, Serialize)]
pub struct PlaybackEndedEvent {
    pub song_id: String,
}

pub fn playback_position_event(snapshot: &PlaybackStateSnapshot) -> PlaybackPositionEvent {
    PlaybackPositionEvent {
        ms: snapshot.position_ms,
        snapshot: snapshot.clone(),
    }
}

#[cfg(test)]
mod tests {
    use super::{
        PLAYBACK_POSITION_POLL_INTERVAL_MS, PlaybackStateSnapshot, StemVolumes,
        playback_position_event,
    };

    #[test]
    fn playback_position_poll_interval_targets_thirty_hz() {
        assert_eq!(PLAYBACK_POSITION_POLL_INTERVAL_MS, 33);
    }

    #[test]
    fn playback_position_event_carries_the_authoritative_snapshot() {
        let snapshot = PlaybackStateSnapshot {
            song_id: Some("song-a".to_owned()),
            state: "playing".to_owned(),
            is_playing: true,
            position_ms: 1_234,
            duration_ms: Some(5_000),
            volume: 0.8,
            stem_volumes: StemVolumes::default(),
            has_stems: false,
            stem_mode: None,
        };

        let event = playback_position_event(&snapshot);

        assert_eq!(event.ms, 1_234);
        assert_eq!(event.snapshot, snapshot);
    }

    #[test]
    fn playback_controller_reports_loading_until_track_starts() {
        let mut controller = super::PlaybackController::default();

        let loading = controller.start_track_loading("song-a");
        assert_eq!(loading.song_id.as_deref(), Some("song-a"));
        assert_eq!(loading.state, "loading");

        let snapshot = controller.snapshot(1_000);
        assert_eq!(snapshot.song_id.as_deref(), Some("song-a"));
        assert_eq!(snapshot.state, "loading");
        assert!(!snapshot.is_playing);

        let decoded = super::DecodedAudio {
            sample_rate: 44_100,
            channels: 2,
            duration_ms: 1_000,
            samples: vec![0.0; 44_100 * 2],
        };
        let started = controller.start_track("song-a".to_owned(), decoded, 1_000);
        assert_eq!(started.state, "playing");
        assert!(started.is_playing);
    }
}
