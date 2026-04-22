use anyhow::{anyhow, bail, Context, Result};
use std::{
    fs::File,
    io::{Cursor, Read, Seek},
    path::Path,
};
use symphonia::core::{
    audio::{AudioBufferRef, SampleBuffer},
    codecs::DecoderOptions,
    errors::Error as SymphoniaError,
    formats::FormatOptions,
    io::{MediaSource, MediaSourceStream},
    meta::MetadataOptions,
    probe::Hint,
};

#[derive(Debug, Clone, PartialEq)]
pub struct DecodedAudio {
    pub sample_rate: u32,
    pub channels: usize,
    pub duration_ms: u64,
    pub samples: Vec<f32>,
}

pub fn decode_file(path: &Path) -> Result<DecodedAudio> {
    let file = File::open(path)
        .with_context(|| format!("failed to open audio file at {}", path.display()))?;
    decode_source(
        file,
        path.extension().and_then(|value| value.to_str()),
        &path.display().to_string(),
    )
}

pub fn decode_bytes(bytes: Vec<u8>, extension: &str) -> Result<DecodedAudio> {
    decode_source(
        Cursor::new(bytes),
        Some(extension),
        "in-memory Media+G audio",
    )
}

pub fn probe_file(path: &Path) -> Result<()> {
    let file = File::open(path)
        .with_context(|| format!("failed to open audio file at {}", path.display()))?;
    probe_source(
        file,
        path.extension().and_then(|value| value.to_str()),
        &path.display().to_string(),
    )
}

pub fn probe_bytes(bytes: Vec<u8>, extension: &str) -> Result<()> {
    probe_source(
        Cursor::new(bytes),
        Some(extension),
        "in-memory Media+G audio",
    )
}

fn extend_interleaved_samples(samples: &mut Vec<f32>, decoded: AudioBufferRef<'_>) {
    let mut sample_buffer = SampleBuffer::<f32>::new(decoded.capacity() as u64, *decoded.spec());
    sample_buffer.copy_interleaved_ref(decoded);
    samples.extend_from_slice(sample_buffer.samples());
}

fn probe_source<R>(source: R, extension: Option<&str>, source_label: &str) -> Result<()>
where
    R: Read + Seek + MediaSource + Send + Sync + 'static,
{
    let media_source_stream = MediaSourceStream::new(Box::new(source), Default::default());

    let mut hint = Hint::new();
    if let Some(extension) = extension {
        hint.with_extension(extension);
    }

    let probed = symphonia::default::get_probe()
        .format(
            &hint,
            media_source_stream,
            &FormatOptions::default(),
            &MetadataOptions::default(),
        )
        .with_context(|| format!("failed to probe audio format for {source_label}"))?;

    probed
        .format
        .default_track()
        .context("audio container does not expose a default track")?;

    Ok(())
}

fn decode_source<R>(source: R, extension: Option<&str>, source_label: &str) -> Result<DecodedAudio>
where
    R: Read + Seek + MediaSource + Send + Sync + 'static,
{
    let media_source_stream = MediaSourceStream::new(Box::new(source), Default::default());

    let mut hint = Hint::new();
    if let Some(extension) = extension {
        hint.with_extension(extension);
    }

    let probed = symphonia::default::get_probe()
        .format(
            &hint,
            media_source_stream,
            &FormatOptions::default(),
            &MetadataOptions::default(),
        )
        .with_context(|| format!("failed to probe audio format for {source_label}"))?;
    let mut format = probed.format;

    let track = format
        .default_track()
        .context("audio container does not expose a default track")?;
    let codec_params = &track.codec_params;
    let mut sample_rate = codec_params.sample_rate;
    let mut channels = codec_params.channels.map(|layout| layout.count());

    let mut decoder = symphonia::default::get_codecs()
        .make(codec_params, &DecoderOptions::default())
        .context("failed to create audio decoder")?;
    let track_id = track.id;
    let mut samples = Vec::new();

    loop {
        let packet = match format.next_packet() {
            Ok(packet) => packet,
            Err(SymphoniaError::IoError(error))
                if error.kind() == std::io::ErrorKind::UnexpectedEof =>
            {
                break;
            }
            Err(SymphoniaError::ResetRequired) => {
                bail!("decoder reset is not supported by the Phase 2 decode pipeline");
            }
            Err(error) => return Err(error).context("failed while reading audio packets"),
        };

        if packet.track_id() != track_id {
            continue;
        }

        let decoded = decoder
            .decode(&packet)
            .with_context(|| format!("failed to decode audio packet from {source_label}"))?;

        let spec = *decoded.spec();
        sample_rate.get_or_insert(spec.rate);
        channels.get_or_insert(spec.channels.count());
        extend_interleaved_samples(&mut samples, decoded);
    }

    if samples.is_empty() {
        return Err(anyhow!("decoded audio contained no PCM samples"));
    }

    let sample_rate = sample_rate.context("audio track is missing sample rate metadata")?;
    let channels = channels.context("audio track is missing channel metadata")?;
    let frame_count = samples.len() / channels;
    let duration_ms = ((frame_count as f64 / sample_rate as f64) * 1000.0).round() as u64;

    Ok(DecodedAudio {
        sample_rate,
        channels,
        duration_ms,
        samples,
    })
}
