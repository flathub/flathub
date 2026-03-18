use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::{
    fs,
    path::{Path, PathBuf},
};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CheckpointManifest {
    pub song_hash: String,
    pub total_chunks: usize,
    pub target_frame_count: usize,
    pub input_frame_count: usize,
    pub channels: usize,
    pub sample_rate: u32,
    pub stem_count: usize,
}

/// Returns the `.chunks/` directory path for a given song.
pub fn checkpoint_dir(stems_base: &Path, song_hash: &str) -> PathBuf {
    stems_base.join(song_hash).join(".chunks")
}

/// Write `manifest.json` at the start of inference.
pub fn write_manifest(dir: &Path, manifest: &CheckpointManifest) -> Result<()> {
    fs::create_dir_all(dir)
        .with_context(|| format!("failed to create checkpoint directory at {}", dir.display()))?;
    let json = serde_json::to_string_pretty(manifest)
        .context("failed to serialize checkpoint manifest")?;
    let path = dir.join("manifest.json");
    fs::write(&path, json)
        .with_context(|| format!("failed to write checkpoint manifest at {}", path.display()))?;
    Ok(())
}

/// Read the manifest, returning `None` if missing or unreadable.
pub fn read_manifest(dir: &Path) -> Result<Option<CheckpointManifest>> {
    let path = dir.join("manifest.json");
    if !path.exists() {
        return Ok(None);
    }
    let json = fs::read_to_string(&path)
        .with_context(|| format!("failed to read checkpoint manifest at {}", path.display()))?;
    let manifest: CheckpointManifest = serde_json::from_str(&json)
        .with_context(|| format!("failed to parse checkpoint manifest at {}", path.display()))?;
    Ok(Some(manifest))
}

/// Write a single chunk's stem data atomically (write to `.tmp` then rename).
///
/// Binary format:
/// - First 4 bytes: `chunk_index` as `u32` LE
/// - Remaining: raw `f32` LE bytes (samples for all stems concatenated)
pub fn write_chunk(dir: &Path, chunk_index: usize, chunk_data: &[f32]) -> Result<()> {
    fs::create_dir_all(dir)
        .with_context(|| format!("failed to create checkpoint directory at {}", dir.display()))?;

    let tmp_path = dir.join(format!("chunk_{:04}.bin.tmp", chunk_index));
    let final_path = dir.join(format!("chunk_{:04}.bin", chunk_index));

    let mut buf = Vec::with_capacity(4 + chunk_data.len() * 4);
    buf.extend_from_slice(&(chunk_index as u32).to_le_bytes());
    for &sample in chunk_data {
        buf.extend_from_slice(&sample.to_le_bytes());
    }

    fs::write(&tmp_path, &buf).with_context(|| {
        format!(
            "failed to write temporary chunk file at {}",
            tmp_path.display()
        )
    })?;
    fs::rename(&tmp_path, &final_path).with_context(|| {
        format!(
            "failed to rename chunk file from {} to {}",
            tmp_path.display(),
            final_path.display()
        )
    })?;

    Ok(())
}

/// List completed chunk indices by scanning the directory for `chunk_NNNN.bin` files.
pub fn list_completed_chunks(dir: &Path) -> Result<Vec<usize>> {
    if !dir.exists() {
        return Ok(Vec::new());
    }
    let mut indices = Vec::new();
    let entries = fs::read_dir(dir)
        .with_context(|| format!("failed to read checkpoint directory at {}", dir.display()))?;
    for entry in entries {
        let entry = entry.context("failed to read directory entry")?;
        let file_name = entry.file_name();
        let name = file_name.to_string_lossy();
        if let Some(index_str) = name
            .strip_prefix("chunk_")
            .and_then(|s| s.strip_suffix(".bin"))
        {
            if let Ok(index) = index_str.parse::<usize>() {
                indices.push(index);
            }
        }
    }
    indices.sort_unstable();
    Ok(indices)
}

/// Read a chunk's data back from its binary file.
pub fn read_chunk(dir: &Path, chunk_index: usize) -> Result<Vec<f32>> {
    let path = dir.join(format!("chunk_{:04}.bin", chunk_index));
    let buf = fs::read(&path)
        .with_context(|| format!("failed to read chunk file at {}", path.display()))?;
    if buf.len() < 4 {
        anyhow::bail!("chunk file {} is too small", path.display());
    }
    let stored_index = u32::from_le_bytes([buf[0], buf[1], buf[2], buf[3]]) as usize;
    if stored_index != chunk_index {
        anyhow::bail!(
            "chunk file {} contains index {stored_index} but expected {chunk_index}",
            path.display()
        );
    }
    let sample_bytes = &buf[4..];
    if sample_bytes.len() % 4 != 0 {
        anyhow::bail!(
            "chunk file {} has invalid sample data length {}",
            path.display(),
            sample_bytes.len()
        );
    }
    let samples: Vec<f32> = sample_bytes
        .chunks_exact(4)
        .map(|chunk| f32::from_le_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]))
        .collect();
    Ok(samples)
}

/// Remove the `.chunks/` directory after successful completion.
pub fn cleanup(dir: &Path) -> Result<()> {
    if dir.exists() {
        fs::remove_dir_all(dir).with_context(|| {
            format!("failed to remove checkpoint directory at {}", dir.display())
        })?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env;

    fn temp_dir(name: &str) -> PathBuf {
        env::temp_dir().join("openkara-checkpoint-tests").join(name)
    }

    fn cleanup_temp(dir: &Path) {
        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn manifest_round_trip() {
        let dir = temp_dir("manifest-round-trip");
        cleanup_temp(&dir);

        let manifest = CheckpointManifest {
            song_hash: "abc123".to_string(),
            total_chunks: 5,
            target_frame_count: 44100,
            input_frame_count: 220500,
            channels: 2,
            sample_rate: 44100,
            stem_count: 4,
        };

        write_manifest(&dir, &manifest).expect("write manifest");
        let read_back = read_manifest(&dir)
            .expect("read manifest")
            .expect("manifest should exist");

        assert_eq!(read_back.song_hash, "abc123");
        assert_eq!(read_back.total_chunks, 5);
        assert_eq!(read_back.stem_count, 4);

        cleanup_temp(&dir);
    }

    #[test]
    fn read_manifest_returns_none_when_missing() {
        let dir = temp_dir("manifest-missing");
        cleanup_temp(&dir);
        let result = read_manifest(&dir).expect("read manifest");
        assert!(result.is_none());
    }

    #[test]
    fn chunk_write_and_read_round_trip() {
        let dir = temp_dir("chunk-round-trip");
        cleanup_temp(&dir);

        let data: Vec<f32> = vec![1.0, 2.0, 3.0, -1.0, 0.5];
        write_chunk(&dir, 3, &data).expect("write chunk");

        let read_back = read_chunk(&dir, 3).expect("read chunk");
        assert_eq!(read_back, data);

        cleanup_temp(&dir);
    }

    #[test]
    fn list_completed_chunks_finds_written_chunks() {
        let dir = temp_dir("list-chunks");
        cleanup_temp(&dir);

        write_chunk(&dir, 0, &[1.0]).expect("write chunk 0");
        write_chunk(&dir, 2, &[2.0]).expect("write chunk 2");
        write_chunk(&dir, 5, &[3.0]).expect("write chunk 5");

        let indices = list_completed_chunks(&dir).expect("list chunks");
        assert_eq!(indices, vec![0, 2, 5]);

        cleanup_temp(&dir);
    }

    #[test]
    fn cleanup_removes_directory() {
        let dir = temp_dir("cleanup-test");
        cleanup_temp(&dir);

        write_chunk(&dir, 0, &[1.0]).expect("write chunk");
        assert!(dir.exists());

        cleanup(&dir).expect("cleanup");
        assert!(!dir.exists());
    }

    #[test]
    fn checkpoint_dir_builds_expected_path() {
        let base = Path::new("/tmp/stems");
        let result = checkpoint_dir(base, "hash123");
        assert_eq!(result, PathBuf::from("/tmp/stems/hash123/.chunks"));
    }
}
