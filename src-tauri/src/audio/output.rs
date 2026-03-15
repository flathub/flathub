use crate::audio::decode::DecodedAudio;
use crate::audio::playback::{monotonic_now_ms, LoadedStems, PlaybackController};
use anyhow::{Context, Result};
use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use cpal::{Sample, SampleFormat, SizedSample, Stream};
use std::{
    sync::{
        atomic::{AtomicBool, Ordering},
        mpsc, Arc, Mutex,
    },
    thread,
    time::Duration,
};

pub fn ensure_output_thread(
    started: &Arc<AtomicBool>,
    start_lock: &Arc<Mutex<()>>,
    playback: Arc<Mutex<PlaybackController>>,
) -> Result<()> {
    if started.load(Ordering::SeqCst) {
        return Ok(());
    }

    let _guard = start_lock
        .lock()
        .map_err(|_| anyhow::anyhow!("audio output start lock was poisoned"))?;
    if started.load(Ordering::SeqCst) {
        return Ok(());
    }

    let (startup_tx, startup_rx) = mpsc::sync_channel(1);
    thread::spawn(move || {
        if let Err(error) = start_output_thread(playback, startup_tx) {
            eprintln!("audio output thread failed to start: {error:#}");
        }
    });

    startup_rx
        .recv_timeout(Duration::from_secs(5))
        .context("timed out while waiting for audio output thread startup")??;
    started.store(true, Ordering::SeqCst);

    Ok(())
}

pub fn render_output_buffer(
    playback: &mut PlaybackController,
    now_ms: u64,
    output: &mut [f32],
    device_sample_rate: u32,
    device_channels: usize,
) -> usize {
    output.fill(0.0);

    let snapshot = playback.snapshot(now_ms);
    if !snapshot.is_playing {
        return 0;
    }

    let Some(track) = &playback.current_track else {
        return 0;
    };

    let master = snapshot.volume;
    let sv = snapshot.stem_volumes;

    if let Some(loaded_stems) = &track.stems {
        let rendered = match loaded_stems {
            LoadedStems::TwoStem {
                vocals,
                accompaniment,
            } => {
                let accomp_gain = sv.drums;
                let mut rendered = 0;
                rendered = rendered.max(mix_stem_resampled(
                    output,
                    vocals,
                    snapshot.position_ms,
                    master * sv.vocals,
                    device_sample_rate,
                    device_channels,
                ));
                rendered = rendered.max(mix_stem_resampled(
                    output,
                    accompaniment,
                    snapshot.position_ms,
                    master * accomp_gain,
                    device_sample_rate,
                    device_channels,
                ));
                rendered
            }
            LoadedStems::FourStem(stems) => {
                let mut rendered = 0;
                rendered = rendered.max(mix_stem_resampled(
                    output,
                    &stems.vocals,
                    snapshot.position_ms,
                    master * sv.vocals,
                    device_sample_rate,
                    device_channels,
                ));
                rendered = rendered.max(mix_stem_resampled(
                    output,
                    &stems.drums,
                    snapshot.position_ms,
                    master * sv.drums,
                    device_sample_rate,
                    device_channels,
                ));
                rendered = rendered.max(mix_stem_resampled(
                    output,
                    &stems.bass,
                    snapshot.position_ms,
                    master * sv.bass,
                    device_sample_rate,
                    device_channels,
                ));
                rendered = rendered.max(mix_stem_resampled(
                    output,
                    &stems.other,
                    snapshot.position_ms,
                    master * sv.other,
                    device_sample_rate,
                    device_channels,
                ));
                rendered
            }
        };

        // Clamp to prevent clipping
        for sample in output.iter_mut() {
            *sample = sample.clamp(-1.0, 1.0);
        }

        rendered
    } else {
        // Fallback: play original audio with master volume
        let original = &track.original_audio;
        mix_stem_resampled(
            output,
            original,
            snapshot.position_ms,
            master,
            device_sample_rate,
            device_channels,
        )
    }
}

/// Mix a single audio source into the output buffer with sample-rate conversion
/// and channel mapping. Uses linear interpolation for resampling.
fn mix_stem_resampled(
    output: &mut [f32],
    audio: &DecodedAudio,
    position_ms: u64,
    gain: f32,
    device_sample_rate: u32,
    device_channels: usize,
) -> usize {
    if gain == 0.0 {
        return 0;
    }

    let src_rate = audio.sample_rate as f64;
    let dst_rate = device_sample_rate as f64;
    let src_channels = audio.channels;
    let total_src_frames = audio.samples.len() / src_channels;

    // Calculate the source frame corresponding to the current playback position
    let src_start_frame = (position_ms as f64 * src_rate / 1000.0) as usize;
    if src_start_frame >= total_src_frames {
        return 0;
    }

    let output_frames = output.len() / device_channels;
    let rate_ratio = src_rate / dst_rate;
    let mut written = 0;

    for out_frame in 0..output_frames {
        // Map output frame to source frame with fractional position
        let src_pos = src_start_frame as f64 + out_frame as f64 * rate_ratio;
        let src_frame_lo = src_pos as usize;

        if src_frame_lo >= total_src_frames {
            break;
        }

        let can_interpolate = src_frame_lo + 1 < total_src_frames;
        let frac = (src_pos - src_frame_lo as f64) as f32;

        for out_ch in 0..device_channels {
            let src_ch = if out_ch < src_channels {
                out_ch
            } else {
                out_ch % src_channels
            };
            let idx_lo = src_frame_lo * src_channels + src_ch;
            let sample = if can_interpolate && frac > 0.0 {
                let idx_hi = (src_frame_lo + 1) * src_channels + src_ch;
                audio.samples[idx_lo] * (1.0 - frac) + audio.samples[idx_hi] * frac
            } else {
                audio.samples[idx_lo]
            };
            output[out_frame * device_channels + out_ch] += sample * gain;
        }

        written = (out_frame + 1) * device_channels;
    }

    written
}

fn build_output_stream<T>(
    device: &cpal::Device,
    config: &cpal::StreamConfig,
    playback: Arc<Mutex<PlaybackController>>,
) -> Result<Stream>
where
    T: SizedSample + Sample + cpal::FromSample<f32>,
{
    let channels = config.channels as usize;
    let sample_rate = config.sample_rate.0;

    let stream = device.build_output_stream(
        config,
        move |data: &mut [T], _info| {
            let mut scratch = vec![0.0_f32; data.len()];

            if let Ok(mut controller) = playback.lock() {
                let rendered_samples = render_output_buffer(
                    &mut controller,
                    monotonic_now_ms(),
                    &mut scratch,
                    sample_rate,
                    channels,
                );

                if rendered_samples < scratch.len() {
                    scratch[rendered_samples..].fill(0.0);
                }
            } else {
                scratch.fill(0.0);
            }

            for frame in scratch.chunks(channels).zip(data.chunks_mut(channels)) {
                let (input_frame, output_frame) = frame;
                for (input_sample, output_sample) in
                    input_frame.iter().zip(output_frame.iter_mut())
                {
                    *output_sample = T::from_sample(*input_sample);
                }
            }
        },
        move |error| {
            eprintln!("audio output stream error: {error}");
        },
        None,
    )?;

    Ok(stream)
}

fn start_output_thread(
    playback: Arc<Mutex<PlaybackController>>,
    startup_tx: mpsc::SyncSender<Result<()>>,
) -> Result<()> {
    let host = cpal::default_host();
    let device = host
        .default_output_device()
        .context("no default output audio device is available")?;
    let config = device
        .default_output_config()
        .context("failed to read default audio output config")?;
    let stream = match config.sample_format() {
        SampleFormat::F32 => build_output_stream::<f32>(&device, &config.into(), playback)?,
        SampleFormat::I16 => build_output_stream::<i16>(&device, &config.into(), playback)?,
        SampleFormat::U16 => build_output_stream::<u16>(&device, &config.into(), playback)?,
        sample_format => {
            anyhow::bail!("unsupported audio output sample format: {sample_format:?}");
        }
    };

    stream
        .play()
        .context("failed to start audio output stream")?;
    let _ = startup_tx.send(Ok(()));

    loop {
        thread::sleep(Duration::from_secs(60));
        let _keep_alive = &stream;
    }
}
