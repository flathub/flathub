use super::decode::DecodedAudio;
use anyhow::{Context, Result};
use audioadapter_buffers::direct::InterleavedSlice;
use rubato::{Fft, FixedSync, Resampler};

/// Resample audio to `target_rate` using FFT-based resampling (rubato).
/// Returns the audio unchanged if the sample rate already matches.
pub fn resample_audio(audio: DecodedAudio, target_rate: u32) -> Result<DecodedAudio> {
    if audio.sample_rate == target_rate {
        return Ok(audio);
    }

    let frame_count = audio.samples.len() / audio.channels;
    let input_adapter = InterleavedSlice::new(&audio.samples, audio.channels, frame_count)
        .context("failed to wrap interleaved audio for resampling")?;

    let mut resampler = Fft::<f32>::new(
        audio.sample_rate as usize,
        target_rate as usize,
        1024,
        2,
        audio.channels,
        FixedSync::Both,
    )
    .with_context(|| {
        format!(
            "failed to create resampler from {} Hz to {} Hz",
            audio.sample_rate, target_rate
        )
    })?;

    let output_frame_capacity = resampler.process_all_needed_output_len(frame_count);
    let mut output_samples = vec![0.0_f32; output_frame_capacity * audio.channels];
    let mut output_adapter =
        InterleavedSlice::new_mut(&mut output_samples, audio.channels, output_frame_capacity)
            .context("failed to prepare output buffer for resampling")?;

    let (_, output_frames) = resampler
        .process_all_into_buffer(&input_adapter, &mut output_adapter, frame_count, None)
        .context("failed while resampling audio for playback")?;

    output_samples.truncate(output_frames * audio.channels);

    Ok(DecodedAudio {
        sample_rate: target_rate,
        channels: audio.channels,
        duration_ms: ((output_frames as f64 / target_rate as f64) * 1000.0).round() as u64,
        samples: output_samples,
    })
}
