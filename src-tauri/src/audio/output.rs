use crate::airplay_stream::AirPlayAudioTap;
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
    airplay_audio_tap: Arc<AirPlayAudioTap>,
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
        if let Err(error) = start_output_thread(playback, airplay_audio_tap, startup_tx) {
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
    let render_frame = track.render_frame;

    let (rendered, src_frames_advanced) = if let Some(loaded_stems) = &track.stems {
        let result = match loaded_stems {
            LoadedStems::TwoStem {
                vocals,
                accompaniment,
            } => {
                let accomp_gain = sv.drums.max(sv.bass).max(sv.other);
                let (r1, f1) = mix_stem_resampled(
                    output,
                    vocals,
                    render_frame,
                    master * sv.vocals,
                    device_sample_rate,
                    device_channels,
                );
                let (r2, f2) = mix_stem_resampled(
                    output,
                    accompaniment,
                    render_frame,
                    master * accomp_gain,
                    device_sample_rate,
                    device_channels,
                );
                (r1.max(r2), f1.max(f2))
            }
            LoadedStems::FourStem(stems) => {
                let (r1, f1) = mix_stem_resampled(
                    output,
                    &stems.vocals,
                    render_frame,
                    master * sv.vocals,
                    device_sample_rate,
                    device_channels,
                );
                let (r2, f2) = mix_stem_resampled(
                    output,
                    &stems.drums,
                    render_frame,
                    master * sv.drums,
                    device_sample_rate,
                    device_channels,
                );
                let (r3, f3) = mix_stem_resampled(
                    output,
                    &stems.bass,
                    render_frame,
                    master * sv.bass,
                    device_sample_rate,
                    device_channels,
                );
                let (r4, f4) = mix_stem_resampled(
                    output,
                    &stems.other,
                    render_frame,
                    master * sv.other,
                    device_sample_rate,
                    device_channels,
                );
                (r1.max(r2).max(r3).max(r4), f1.max(f2).max(f3).max(f4))
            }
        };

        // Clamp to prevent clipping
        for sample in output.iter_mut() {
            *sample = sample.clamp(-1.0, 1.0);
        }

        result
    } else {
        // Fallback: play original audio with master volume
        let original = &track.original_audio;
        let result = mix_stem_resampled(
            output,
            original,
            render_frame,
            master,
            device_sample_rate,
            device_channels,
        );

        // Clamp to prevent clipping
        for sample in output.iter_mut() {
            *sample = sample.clamp(-1.0, 1.0);
        }

        result
    };

    // Advance the render frame counter so the next callback continues seamlessly
    playback.advance_render_frame(src_frames_advanced);

    rendered
}

/// Mix a single audio source into the output buffer with sample-rate conversion
/// and channel mapping. Uses linear interpolation for resampling.
///
/// Returns `(written_output_samples, source_frames_consumed)`.
fn mix_stem_resampled(
    output: &mut [f32],
    audio: &DecodedAudio,
    start_frame: u64,
    gain: f32,
    device_sample_rate: u32,
    device_channels: usize,
) -> (usize, u64) {
    if gain == 0.0 {
        return (0, 0);
    }

    // Most desktop devices run the same 44.1 kHz rate as the source media.
    // Skipping interpolation in that common case removes hot-path math without
    // changing channel mapping or render-frame progression semantics.
    if audio.sample_rate == device_sample_rate {
        return mix_stem_same_rate(output, audio, start_frame, gain, device_channels);
    }

    mix_stem_linearly_resampled(
        output,
        audio,
        start_frame,
        gain,
        device_sample_rate,
        device_channels,
    )
}

fn mix_stem_same_rate(
    output: &mut [f32],
    audio: &DecodedAudio,
    start_frame: u64,
    gain: f32,
    device_channels: usize,
) -> (usize, u64) {
    let src_channels = audio.channels;
    let total_src_frames = audio.samples.len() / src_channels;
    let src_start_frame = start_frame as usize;
    if src_start_frame >= total_src_frames {
        return (0, 0);
    }

    let output_frames = output.len() / device_channels;
    let available_frames = (total_src_frames - src_start_frame).min(output_frames);

    for out_frame in 0..available_frames {
        let src_frame = src_start_frame + out_frame;
        for out_ch in 0..device_channels {
            let src_ch = if out_ch < src_channels {
                out_ch
            } else {
                out_ch % src_channels
            };
            let sample = audio.samples[src_frame * src_channels + src_ch];
            output[out_frame * device_channels + out_ch] += sample * gain;
        }
    }

    (available_frames * device_channels, available_frames as u64)
}

fn mix_stem_linearly_resampled(
    output: &mut [f32],
    audio: &DecodedAudio,
    start_frame: u64,
    gain: f32,
    device_sample_rate: u32,
    device_channels: usize,
) -> (usize, u64) {
    if gain == 0.0 {
        return (0, 0);
    }

    let src_rate = audio.sample_rate as f64;
    let dst_rate = device_sample_rate as f64;
    let src_channels = audio.channels;
    let total_src_frames = audio.samples.len() / src_channels;

    let src_start_frame = start_frame as usize;
    if src_start_frame >= total_src_frames {
        return (0, 0);
    }

    let output_frames = output.len() / device_channels;
    let rate_ratio = src_rate / dst_rate;
    let mut written = 0;
    let mut rendered_out_frames: usize = 0;

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

        rendered_out_frames = out_frame + 1;
        written = rendered_out_frames * device_channels;
    }

    // Calculate how many source frames the next call should skip over.
    // This must match precisely so consecutive buffers join seamlessly.
    let src_frames_consumed = (rendered_out_frames as f64 * rate_ratio).round() as u64;

    (written, src_frames_consumed)
}

fn build_output_stream<T>(
    device: &cpal::Device,
    config: &cpal::StreamConfig,
    playback: Arc<Mutex<PlaybackController>>,
    airplay_audio_tap: Arc<AirPlayAudioTap>,
) -> Result<Stream>
where
    T: SizedSample + Sample + cpal::FromSample<f32>,
{
    let channels = config.channels as usize;
    let sample_rate = config.sample_rate.0;
    let mut scratch = Vec::<f32>::new();

    let stream = device.build_output_stream(
        config,
        move |data: &mut [T], _info| {
            // The audio callback is a realtime path. Reallocating a fresh scratch
            // buffer every device tick can introduce allocator stalls and audible
            // glitches, so the closure keeps one buffer and resizes only when the
            // device changes its callback frame count.
            scratch.resize(data.len(), 0.0);

            if let Ok(mut controller) = playback.lock() {
                let _rendered_samples = render_output_buffer(
                    &mut controller,
                    monotonic_now_ms(),
                    &mut scratch,
                    sample_rate,
                    channels,
                );
            } else {
                scratch.fill(0.0);
            }

            let tap_samples = downmix_for_airplay(&scratch, channels);
            airplay_audio_tap.push_interleaved(sample_rate, 2, &tap_samples);

            for frame in scratch.chunks(channels).zip(data.chunks_mut(channels)) {
                let (input_frame, output_frame) = frame;
                for (input_sample, output_sample) in input_frame.iter().zip(output_frame.iter_mut())
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
    airplay_audio_tap: Arc<AirPlayAudioTap>,
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
        SampleFormat::F32 => {
            build_output_stream::<f32>(&device, &config.into(), playback, airplay_audio_tap)?
        }
        SampleFormat::I16 => {
            build_output_stream::<i16>(&device, &config.into(), playback, airplay_audio_tap)?
        }
        SampleFormat::U16 => {
            build_output_stream::<u16>(&device, &config.into(), playback, airplay_audio_tap)?
        }
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

fn downmix_for_airplay(samples: &[f32], channels: usize) -> Vec<f32> {
    if channels == 0 || samples.is_empty() {
        return Vec::new();
    }

    let mut stereo = Vec::with_capacity((samples.len() / channels).saturating_mul(2));
    for frame in samples.chunks(channels) {
        let left = frame[0];
        let right = if channels == 1 { frame[0] } else { frame[1] };
        stereo.push(left);
        stereo.push(right);
    }

    stereo
}
