use crate::{
    audio::playback::{PlaybackController, PLAYBACK_POSITION_POLL_INTERVAL_MS},
    cache,
    library_root::LibraryRoot,
    services::playback_source::{load_song_audio, probe_song_audio},
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use serde::Serialize;
use std::{fs, path::Path, time::Instant};

pub const PLAYBACK_DB_LOOKUP_LATENCY_THRESHOLD_MS: f64 = 25.0;
pub const PLAYBACK_METADATA_PROBE_LATENCY_THRESHOLD_MS: f64 = 100.0;
pub const PLAYBACK_FULL_DECODE_LATENCY_THRESHOLD_MS: f64 = 1_000.0;
pub const SEEK_LATENCY_THRESHOLD_MS: f64 = 200.0;
pub const LYRICS_JITTER_THRESHOLD_MS: u64 = 50;

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct PerformanceReport {
    pub playback: PlaybackPerformanceReport,
    pub lyrics_sync: LyricsSyncPerformanceReport,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct PlaybackPerformanceReport {
    pub track_db_lookup_latency_ms: f64,
    pub track_metadata_probe_latency_ms: f64,
    pub track_full_decode_latency_ms: f64,
    pub seek_latency_avg_ms: f64,
    pub seek_latency_p95_ms: f64,
    pub seek_latency_max_ms: f64,
    pub seek_samples: usize,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct LyricsSyncPerformanceReport {
    pub position_event_interval_ms: u64,
    pub jitter_budget_ms: u64,
}

pub fn build_backend_performance_report(
    connection: &Connection,
    library_root: &LibraryRoot,
    song_id: &str,
    seek_iterations: usize,
) -> Result<PerformanceReport> {
    // Playback is an eager full decode into memory, so cold-path variance is
    // concentrated in the decode step instead of the DB lookup or controller swap.
    let mut track_load = measure_track_load_breakdown(connection, library_root, song_id, 1_000)?;

    let seek_samples = measure_seek_latencies(
        &mut track_load.playback,
        track_load.duration_ms,
        seek_iterations,
    )
        .context("failed to profile playback seek latency")?;

    Ok(PerformanceReport {
        playback: PlaybackPerformanceReport {
            track_db_lookup_latency_ms: track_load.db_lookup_latency_ms,
            track_metadata_probe_latency_ms: track_load.metadata_probe_latency_ms,
            track_full_decode_latency_ms: track_load.full_decode_latency_ms,
            seek_latency_avg_ms: average(&seek_samples),
            seek_latency_p95_ms: percentile(&seek_samples, 0.95),
            seek_latency_max_ms: seek_samples.iter().copied().fold(0.0_f64, f64::max),
            seek_samples: seek_samples.len(),
        },
        // The frontend sync loop is implemented by the UI agent, not Rust. The
        // backend still defines the timing floor: lyrics can only react as often
        // as `playback-position` is emitted, so the emitter cadence is the raw
        // jitter budget that downstream interpolation has to beat.
        lyrics_sync: LyricsSyncPerformanceReport {
            position_event_interval_ms: PLAYBACK_POSITION_POLL_INTERVAL_MS,
            jitter_budget_ms: PLAYBACK_POSITION_POLL_INTERVAL_MS,
        },
    })
}

struct TrackLoadBreakdown {
    db_lookup_latency_ms: f64,
    metadata_probe_latency_ms: f64,
    full_decode_latency_ms: f64,
    duration_ms: u64,
    playback: PlaybackController,
}

fn measure_track_load_breakdown(
    connection: &Connection,
    library_root: &LibraryRoot,
    song_id: &str,
    now_ms: u64,
) -> Result<TrackLoadBreakdown> {
    let mut playback = PlaybackController::default();
    let lookup_start = Instant::now();
    let song = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;
    let db_lookup_latency_ms = elapsed_ms(lookup_start.elapsed());

    let probe_start = Instant::now();
    probe_song_audio(library_root, &song)?;
    let metadata_probe_latency_ms = elapsed_ms(probe_start.elapsed());

    let decode_start = Instant::now();
    let decoded_audio = load_song_audio(library_root, &song)?;
    let full_decode_latency_ms = elapsed_ms(decode_start.elapsed());
    let duration_ms = decoded_audio.duration_ms;

    playback.start_track(song.hash, decoded_audio, now_ms);

    Ok(TrackLoadBreakdown {
        db_lookup_latency_ms,
        metadata_probe_latency_ms,
        full_decode_latency_ms,
        duration_ms,
        playback,
    })
}

pub fn write_report_json(report: &PerformanceReport, path: &Path) -> Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).with_context(|| {
            format!(
                "failed to create performance report directory {}",
                parent.display()
            )
        })?;
    }

    let json = serde_json::to_string_pretty(report).context("failed to serialize perf report")?;
    fs::write(path, json)
        .with_context(|| format!("failed to write performance report to {}", path.display()))?;

    Ok(())
}

fn measure_seek_latencies(
    playback: &mut PlaybackController,
    duration_ms: u64,
    seek_iterations: usize,
) -> Result<Vec<f64>> {
    let duration_ms = duration_ms.max(1);
    let iterations = seek_iterations.max(1);
    let mut latencies = Vec::with_capacity(iterations);

    for iteration in 0..iterations {
        let target_ms = (iteration as u64 * 97) % duration_ms;
        let now_ms = 10_000 + iteration as u64 * 13;
        let started_at = Instant::now();
        playback.seek(target_ms, now_ms)?;
        latencies.push(elapsed_ms(started_at.elapsed()));
    }

    Ok(latencies)
}

fn elapsed_ms(duration: std::time::Duration) -> f64 {
    duration.as_secs_f64() * 1_000.0
}

fn average(samples: &[f64]) -> f64 {
    samples.iter().sum::<f64>() / samples.len() as f64
}

fn percentile(samples: &[f64], percentile: f64) -> f64 {
    let mut ordered = samples.to_vec();
    ordered.sort_by(|left, right| left.total_cmp(right));
    let last_index = ordered.len().saturating_sub(1);
    let index = ((last_index as f64) * percentile).round() as usize;

    ordered[index.min(last_index)]
}
