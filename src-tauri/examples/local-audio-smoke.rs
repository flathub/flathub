use anyhow::{bail, Result};
use openkara_lib::smoke::{run_local_audio_smoke, LocalAudioSmokeConfig, SeparationSmokeMode};
use std::{env, path::PathBuf};

fn main() -> Result<()> {
    let mut input_dir = None;
    let mut output_dir = None;
    let mut separation_mode = SeparationSmokeMode::Auto;

    for argument in env::args().skip(1) {
        match argument.as_str() {
            "--skip-separation" => separation_mode = SeparationSmokeMode::Disabled,
            _ if input_dir.is_none() => input_dir = Some(PathBuf::from(argument)),
            _ if output_dir.is_none() => output_dir = Some(PathBuf::from(argument)),
            _ => bail!(usage()),
        }
    }

    let input_dir = input_dir.ok_or_else(usage_error)?;
    let output_dir = output_dir.ok_or_else(usage_error)?;
    let report = run_local_audio_smoke(LocalAudioSmokeConfig {
        input_dir,
        output_dir,
        separation_mode,
        seek_iterations: 32,
    })?;

    println!(
        "local audio smoke complete\njson: {}\nmarkdown: {}",
        report.report_json_path.display(),
        report.report_markdown_path.display()
    );

    Ok(())
}

fn usage() -> String {
    "usage: cargo run --example local-audio-smoke -- <input-dir> <output-dir> [--skip-separation]"
        .to_string()
}

fn usage_error() -> anyhow::Error {
    anyhow::anyhow!(usage())
}
