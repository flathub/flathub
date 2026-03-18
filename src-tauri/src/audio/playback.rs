use crate::audio::decode::DecodedAudio;
use anyhow::{bail, Result};
use serde::{Deserialize, Serialize};
use std::sync::OnceLock;
use std::time::Instant;

pub const PLAYBACK_POSITION_EVENT: &str = "playback-position";
pub const PLAYBACK_ENDED_EVENT: &str = "playback-ended";
pub const PLAYBACK_POSITION_POLL_INTERVAL_MS: u64 = 16;

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
    pub is_playing: bool,
    pub position_ms: u64,
    pub duration_ms: Option<u64>,
    pub volume: f32,
    pub stem_volumes: StemVolumes,
    pub has_stems: bool,
    pub stem_mode: Option<String>,
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
    volume: f32,
    stem_volumes: StemVolumes,
}

impl Default for PlaybackController {
    fn default() -> Self {
        Self {
            current_track: None,
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
            let position_ms = track.position_ms(now_ms);
            let duration_ms = track.duration_ms();

            if position_ms >= duration_ms {
                track.base_position_ms = duration_ms;
                track.started_at_ms = None;
            }

            let stem_mode = track.stems.as_ref().map(|s| match s {
                LoadedStems::TwoStem { .. } => "two_stem".to_owned(),
                LoadedStems::FourStem(_) => "four_stem".to_owned(),
            });

            return PlaybackStateSnapshot {
                song_id: Some(track.song_id.clone()),
                is_playing: track.started_at_ms.is_some(),
                position_ms: track.position_ms(now_ms),
                duration_ms: Some(duration_ms),
                volume: self.volume,
                stem_volumes: self.stem_volumes,
                has_stems: track.stems.is_some(),
                stem_mode,
            };
        }

        PlaybackStateSnapshot {
            song_id: None,
            is_playing: false,
            position_ms: 0,
            duration_ms: None,
            volume: self.volume,
            stem_volumes: self.stem_volumes,
            has_stems: false,
            stem_mode: None,
        }
    }

    pub fn current_song_id(&self) -> Option<&str> {
        self.current_track
            .as_ref()
            .map(|track| track.song_id.as_str())
    }

    pub fn clear_track(&mut self) {
        self.current_track = None;
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

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize)]
pub struct PlaybackPositionEvent {
    pub ms: u64,
}

#[derive(Debug, Clone, Serialize)]
pub struct PlaybackEndedEvent {
    pub song_id: String,
}

pub fn playback_position_event(snapshot: &PlaybackStateSnapshot) -> Result<PlaybackPositionEvent> {
    if snapshot.song_id.is_none() {
        bail!("cannot emit playback position without a loaded track");
    }

    Ok(PlaybackPositionEvent {
        ms: snapshot.position_ms,
    })
}
