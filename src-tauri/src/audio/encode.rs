use crate::audio::decode::DecodedAudio;
use anyhow::{Context, Result};
use std::num::{NonZeroU32, NonZeroU8};
use std::path::Path;
use vorbis_rs::{VorbisBitrateManagementStrategy, VorbisEncoderBuilder};

/// Default Vorbis quality setting (~320 kbps for stereo at 44100 Hz).
const DEFAULT_VORBIS_QUALITY: f32 = 0.9;

/// Recommended chunk size (in frames) when feeding audio to the Vorbis encoder.
/// 1024 is the value recommended by the libvorbis documentation.
const ENCODE_CHUNK_FRAMES: usize = 1024;

/// Write audio data as an OGG/Vorbis file.
///
/// OpenKara stores generated stems as OGG on purpose: the library keeps the
/// original source media separately, so the cache format is optimized for
/// space efficiency instead of lossless archival quality.
pub fn write_ogg_file(path: &Path, audio: &DecodedAudio) -> Result<()> {
    write_ogg_file_with_quality(path, audio, DEFAULT_VORBIS_QUALITY)
}

/// Write audio data as an OGG/Vorbis file with configurable quality.
///
/// Quality ranges from -0.1 (lowest, ~45 kbps) to 1.0 (highest, ~500 kbps).
/// Recommended values: 0.4 (~128 kbps), 0.5 (~160 kbps), 0.6 (~192 kbps).
pub fn write_ogg_file_with_quality(path: &Path, audio: &DecodedAudio, quality: f32) -> Result<()> {
    // Ensure the parent directory exists.
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent)
            .with_context(|| format!("failed to create directory {}", parent.display()))?;
    }

    let channels = audio.channels;
    let sample_rate = NonZeroU32::new(audio.sample_rate).context("sample rate must be non-zero")?;
    let channel_count =
        NonZeroU8::try_from(u8::try_from(channels).context("channel count exceeds u8 range")?)
            .context("channel count must be non-zero")?;

    let out_file = std::fs::File::create(path)
        .with_context(|| format!("failed to create OGG file at {}", path.display()))?;
    let writer = std::io::BufWriter::new(out_file);

    let mut encoder = VorbisEncoderBuilder::new(sample_rate, channel_count, writer)
        .context("failed to create Vorbis encoder builder")?
        .bitrate_management_strategy(VorbisBitrateManagementStrategy::QualityVbr {
            target_quality: quality,
        })
        .build()
        .context("failed to build Vorbis encoder")?;

    // Convert interleaved samples to planar format and feed in chunks.
    let total_frames = audio.samples.len() / channels;
    let mut offset = 0;

    while offset < total_frames {
        let chunk_frames = ENCODE_CHUNK_FRAMES.min(total_frames - offset);

        // Build planar buffers: one Vec<f32> per channel.
        let planar: Vec<Vec<f32>> = (0..channels)
            .map(|ch| {
                (0..chunk_frames)
                    .map(|frame| audio.samples[(offset + frame) * channels + ch])
                    .collect()
            })
            .collect();

        encoder
            .encode_audio_block(planar)
            .context("failed to encode audio block")?;

        offset += chunk_frames;
    }

    encoder
        .finish()
        .context("failed to finish Vorbis encoding")?;

    Ok(())
}
