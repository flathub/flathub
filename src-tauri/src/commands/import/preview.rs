use crate::metadata;
use anyhow::{Context, Result};
use std::path::PathBuf;

use super::types::ImportCandidateDetails;

pub(super) fn inspect_import_candidate(path: &str) -> Result<ImportCandidateDetails> {
    let source = PathBuf::from(path);
    let metadata = metadata::read_from_path(&source)?;
    let file_size = std::fs::metadata(&source)
        .with_context(|| format!("failed to inspect import candidate at {}", source.display()))?
        .len();
    let ext = source.extension().and_then(|e| e.to_str()).unwrap_or("bin");
    let bit_rate = if metadata.duration_ms > 0 {
        let duration_secs = metadata.duration_ms as f64 / 1000.0;
        Some(((file_size as f64 * 8.0) / duration_secs / 1000.0).round() as u32)
    } else {
        None
    };

    Ok(ImportCandidateDetails {
        path: path.to_owned(),
        format: display_audio_format(ext).to_owned(),
        bit_rate,
        file_size,
        duration_ms: Some(metadata.duration_ms),
    })
}

pub(super) fn display_audio_format(ext: &str) -> &str {
    match ext.to_lowercase().as_str() {
        "mp3" => "MP3",
        "flac" => "FLAC",
        "wav" | "wave" => "WAV",
        "ogg" => "OGG",
        "aac" | "m4a" => "AAC/M4A",
        "opus" => "Opus",
        "aiff" | "aif" => "AIFF",
        _ => ext,
    }
}
