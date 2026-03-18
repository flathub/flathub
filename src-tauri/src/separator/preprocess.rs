use crate::{audio::decode::DecodedAudio, separator::model::LoadedModel};
use anyhow::{bail, Context, Result};
use audioadapter_buffers::direct::InterleavedSlice;
use rubato::{Fft, FixedSync, Resampler};

pub const DEMUCS_SAMPLE_RATE: u32 = 44_100;
pub const DEMUCS_CHANNELS: usize = 2;

#[derive(Debug, Clone, PartialEq)]
pub struct PreparedModelInput {
    pub shape: Vec<i64>,
    pub samples: Vec<f32>,
}

pub fn target_frame_count(model: &LoadedModel, fallback_frame_count: usize) -> Result<usize> {
    if model.input_shape.len() != 3 {
        bail!(
            "Demucs model input rank must be 3, got {} dimensions",
            model.input_shape.len()
        );
    }

    Ok(match model.input_shape.get(2).copied() {
        Some(frame_count_hint) if frame_count_hint > 0 => frame_count_hint as usize,
        _ => fallback_frame_count,
    })
}

pub fn normalize_audio_for_model(decoded_audio: &DecodedAudio) -> Result<DecodedAudio> {
    if decoded_audio.channels != DEMUCS_CHANNELS {
        bail!(
            "Demucs preprocessing currently requires stereo audio, got {} channels",
            decoded_audio.channels
        );
    }

    if decoded_audio.sample_rate == DEMUCS_SAMPLE_RATE {
        return Ok(decoded_audio.clone());
    }

    let frame_count = decoded_audio.samples.len() / decoded_audio.channels;
    let input_adapter =
        InterleavedSlice::new(&decoded_audio.samples, decoded_audio.channels, frame_count)
            .context("failed to wrap interleaved audio for resampling")?;
    let mut resampler = Fft::<f32>::new(
        decoded_audio.sample_rate as usize,
        DEMUCS_SAMPLE_RATE as usize,
        1024,
        2,
        decoded_audio.channels,
        FixedSync::Both,
    )
    .with_context(|| {
        format!(
            "failed to create resampler from {} Hz to {} Hz",
            decoded_audio.sample_rate, DEMUCS_SAMPLE_RATE
        )
    })?;
    let output_frame_capacity = resampler.process_all_needed_output_len(frame_count);
    let mut output_samples = vec![0.0_f32; output_frame_capacity * decoded_audio.channels];
    let mut output_adapter = InterleavedSlice::new_mut(
        &mut output_samples,
        decoded_audio.channels,
        output_frame_capacity,
    )
    .context("failed to prepare output buffer for resampling")?;
    let (_, output_frames) = resampler
        .process_all_into_buffer(&input_adapter, &mut output_adapter, frame_count, None)
        .context("failed while resampling audio for Demucs preprocessing")?;
    output_samples.truncate(output_frames * decoded_audio.channels);

    Ok(DecodedAudio {
        sample_rate: DEMUCS_SAMPLE_RATE,
        channels: decoded_audio.channels,
        duration_ms: ((output_frames as f64 / DEMUCS_SAMPLE_RATE as f64) * 1000.0).round() as u64,
        samples: output_samples,
    })
}

pub fn prepare_model_input(
    model: &LoadedModel,
    decoded_audio: &DecodedAudio,
) -> Result<PreparedModelInput> {
    let decoded_audio = normalize_audio_for_model(decoded_audio)?;
    prepare_model_input_from_normalized(model, &decoded_audio)
}

pub fn prepare_model_input_from_normalized(
    model: &LoadedModel,
    decoded_audio: &DecodedAudio,
) -> Result<PreparedModelInput> {
    // Chunked separation reuses one normalized buffer across many windows.
    // Keeping a direct entrypoint avoids cloning the full PCM buffer every
    // time a window is prepared for inference.
    if decoded_audio.sample_rate != DEMUCS_SAMPLE_RATE {
        bail!(
            "Demucs preprocessing expects {} Hz normalized audio, got {} Hz",
            DEMUCS_SAMPLE_RATE,
            decoded_audio.sample_rate
        );
    }

    if decoded_audio.channels != DEMUCS_CHANNELS {
        bail!(
            "Demucs preprocessing expects {} channels after normalization, got {}",
            DEMUCS_CHANNELS,
            decoded_audio.channels
        );
    }

    let frame_count = decoded_audio.samples.len() / decoded_audio.channels;
    let target_frame_count = target_frame_count(model, frame_count)?;

    if frame_count > target_frame_count {
        bail!(
            "Demucs preprocessing currently supports audio up to {target_frame_count} frames, got {frame_count}"
        );
    }

    let mut channels_first = vec![0.0_f32; decoded_audio.channels * target_frame_count];

    for frame_index in 0..frame_count {
        let interleaved_offset = frame_index * decoded_audio.channels;
        for channel_index in 0..decoded_audio.channels {
            let channels_first_offset = channel_index * target_frame_count + frame_index;
            channels_first[channels_first_offset] =
                decoded_audio.samples[interleaved_offset + channel_index];
        }
    }

    Ok(PreparedModelInput {
        shape: vec![1, decoded_audio.channels as i64, target_frame_count as i64],
        samples: channels_first,
    })
}
