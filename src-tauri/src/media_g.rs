use anyhow::{bail, Context, Result};
use crate::hash;
use sha2::{Digest, Sha256};
use std::{
    fs::File,
    io::{Read, Seek},
    path::Path,
};
use zip::ZipArchive;

pub const MEDIA_G_PAIRED: &str = "paired";
pub const MEDIA_G_ZIP: &str = "zip";

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ZipMediaGAsset {
    pub audio_entry_name: String,
    pub audio_extension: String,
    pub audio_bytes: Vec<u8>,
    pub cdg_entry_name: String,
    pub cdg_bytes: Vec<u8>,
    pub display_stem: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ArchiveEntry {
    index: usize,
    file_name: String,
    stem_lower: String,
    stem_display: String,
    extension: String,
}

pub fn is_media_g_container(value: Option<&str>) -> bool {
    matches!(value, Some(MEDIA_G_PAIRED | MEDIA_G_ZIP))
}

pub fn is_audio_extension(ext: &str) -> bool {
    matches!(
        ext.to_ascii_lowercase().as_str(),
        "mp3" | "flac" | "wav" | "ogg" | "m4a" | "aac" | "wma"
    )
}

pub fn is_cdg_extension(ext: &str) -> bool {
    ext.eq_ignore_ascii_case("cdg")
}

pub fn inspect_zip_for_media_g(path: &Path) -> Result<ZipMediaGAsset> {
    let file = File::open(path)
        .with_context(|| format!("failed to open Media+G ZIP at {}", path.display()))?;
    let mut archive = ZipArchive::new(file)
        .with_context(|| format!("failed to read ZIP at {}", path.display()))?;

    let mut audio_entries = Vec::new();
    let mut cdg_entries = Vec::new();

    for index in 0..archive.len() {
        let entry = archive.by_index(index).with_context(|| {
            format!("failed to inspect ZIP entry #{index} in {}", path.display())
        })?;
        if entry.is_dir() {
            continue;
        }

        let entry_path = Path::new(entry.name());
        let Some(file_name) = entry_path.file_name().and_then(|value| value.to_str()) else {
            continue;
        };
        let Some(stem) = entry_path.file_stem().and_then(|value| value.to_str()) else {
            continue;
        };
        let Some(extension) = entry_path.extension().and_then(|value| value.to_str()) else {
            continue;
        };

        let extension = extension.to_ascii_lowercase();
        let record = ArchiveEntry {
            index,
            file_name: file_name.to_owned(),
            stem_lower: stem.to_ascii_lowercase(),
            stem_display: stem.to_owned(),
            extension: extension.clone(),
        };

        if is_audio_extension(&extension) {
            audio_entries.push(record);
        } else if is_cdg_extension(&extension) {
            cdg_entries.push(record);
        }
    }

    if audio_entries.is_empty() || cdg_entries.is_empty() {
        bail!(
            "Media+G ZIP {} must contain one audio file and one matching .cdg file",
            path.display()
        );
    }

    let mut matched_pairs = Vec::new();
    for audio in &audio_entries {
        if let Some(cdg) = cdg_entries
            .iter()
            .find(|entry| entry.stem_lower == audio.stem_lower)
        {
            matched_pairs.push((audio.clone(), cdg.clone()));
        }
    }

    if matched_pairs.len() != 1 || audio_entries.len() != 1 || cdg_entries.len() != 1 {
        bail!(
            "Media+G ZIP {} must contain exactly one audio/.cdg pair",
            path.display()
        );
    }

    let (audio_entry, cdg_entry) = matched_pairs.pop().expect("one pair should exist");
    let audio_bytes = read_zip_entry_bytes(&mut archive, audio_entry.index)
        .with_context(|| format!("failed to read audio from {}", path.display()))?;
    let cdg_bytes = read_zip_entry_bytes(&mut archive, cdg_entry.index)
        .with_context(|| format!("failed to read CDG data from {}", path.display()))?;

    Ok(ZipMediaGAsset {
        audio_entry_name: audio_entry.file_name,
        audio_extension: audio_entry.extension,
        audio_bytes,
        cdg_entry_name: cdg_entry.file_name,
        cdg_bytes,
        display_stem: audio_entry.stem_display,
    })
}

pub fn media_g_hash(audio_bytes: &[u8], cdg_bytes: &[u8]) -> String {
    let mut hasher = Sha256::new();
    // Hash the logical pair instead of the transport container so a ZIP and a
    // loose file pair for the same karaoke asset resolve to the same library ID.
    hasher.update(b"audio\0");
    hasher.update(audio_bytes);
    hasher.update(b"cdg\0");
    hasher.update(cdg_bytes);
    hash::hex_lower(hasher.finalize())
}

fn read_zip_entry_bytes<R>(archive: &mut ZipArchive<R>, index: usize) -> Result<Vec<u8>>
where
    R: Read + Seek,
{
    let mut entry = archive
        .by_index(index)
        .with_context(|| format!("failed to reopen ZIP entry #{index}"))?;
    let mut bytes = Vec::new();
    entry
        .read_to_end(&mut bytes)
        .with_context(|| format!("failed to read ZIP entry {}", entry.name()))?;
    Ok(bytes)
}
