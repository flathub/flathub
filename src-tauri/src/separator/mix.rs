use crate::{
    audio::decode::DecodedAudio,
    separator::inference::{SeparatedStem, SeparationResult},
};
use anyhow::{bail, Context, Result};
use std::path::Path;

const ACCOMPANIMENT_STEM_NAMES: [&str; 3] = ["drums", "bass", "other"];

pub fn mix_accompaniment(separation: &SeparationResult) -> Result<DecodedAudio> {
    let stems = ACCOMPANIMENT_STEM_NAMES
        .iter()
        .map(|name| find_stem(&separation.stems, name))
        .collect::<Result<Vec<_>>>()?;
    let reference_audio = &stems[0].audio;
    validate_consistent_audio_shape(&stems, reference_audio)?;

    let mut mixed_samples = vec![0.0_f32; reference_audio.samples.len()];
    for stem in stems {
        for (mixed_sample, stem_sample) in mixed_samples.iter_mut().zip(&stem.audio.samples) {
            *mixed_sample += stem_sample;
        }
    }

    let peak = mixed_samples
        .iter()
        .map(|sample| sample.abs())
        .fold(0.0_f32, f32::max);

    // Summing three stems can exceed the unit range expected by later encoding.
    if peak > 1.0 {
        for sample in &mut mixed_samples {
            *sample /= peak;
        }
    }

    Ok(DecodedAudio {
        sample_rate: reference_audio.sample_rate,
        channels: reference_audio.channels,
        duration_ms: reference_audio.duration_ms,
        samples: mixed_samples,
    })
}

pub fn write_accompaniment_ogg(audio: &DecodedAudio, path: &Path) -> Result<()> {
    crate::audio::encode::write_ogg_file(path, audio)
}

fn find_stem<'a>(stems: &'a [SeparatedStem], name: &str) -> Result<&'a SeparatedStem> {
    stems
        .iter()
        .find(|stem| stem.name == name)
        .with_context(|| format!("missing required accompaniment stem {name}"))
}

fn validate_consistent_audio_shape(
    stems: &[&SeparatedStem],
    reference_audio: &DecodedAudio,
) -> Result<()> {
    for stem in stems.iter().skip(1) {
        if stem.audio.sample_rate != reference_audio.sample_rate {
            bail!(
                "accompaniment stems must share a sample rate, {} had {} Hz and reference was {} Hz",
                stem.name,
                stem.audio.sample_rate,
                reference_audio.sample_rate
            );
        }
        if stem.audio.channels != reference_audio.channels {
            bail!(
                "accompaniment stems must share a channel count, {} had {} and reference was {}",
                stem.name,
                stem.audio.channels,
                reference_audio.channels
            );
        }
        if stem.audio.samples.len() != reference_audio.samples.len() {
            bail!(
                "accompaniment stems must share a sample length, {} had {} samples and reference had {}",
                stem.name,
                stem.audio.samples.len(),
                reference_audio.samples.len()
            );
        }
    }

    Ok(())
}
