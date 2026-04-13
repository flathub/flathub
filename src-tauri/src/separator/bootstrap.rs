use crate::config::ModelVariant;
use anyhow::{bail, Context, Result};
use reqwest::blocking::Client;
use sha2::{Digest, Sha256};
use std::{
    fs,
    io::Read,
    path::{Path, PathBuf},
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

pub struct ModelDescriptor {
    pub filename: &'static str,
    pub download_url: &'static str,
    pub sha256: &'static str,
}

pub const HTDEMUCS: ModelDescriptor = ModelDescriptor {
    filename: "htdemucs.onnx",
    download_url: "https://github.com/thedavidweng/openkara-models/releases/download/model-v2.0.1/htdemucs.onnx",
    sha256: "8fa3dab679c59aeb049dd229f57a212c9339b3fc17ebf50541daad9e799364a1",
};

pub const HTDEMUCS_FT: ModelDescriptor = ModelDescriptor {
    filename: "htdemucs_ft.onnx",
    download_url: "https://github.com/thedavidweng/openkara-models/releases/download/model-ft-v2.0.1/htdemucs_ft.onnx",
    sha256: "0f2efbd7044182c10a6e8169b670392a3a91f904635e29329d6a3667375f5c94",
};

/// Backward-compatible aliases used by existing code and tests.
pub const MODEL_DOWNLOAD_URL: &str = HTDEMUCS.download_url;
pub const MODEL_SHA256: &str = HTDEMUCS.sha256;

pub fn descriptor_for(variant: ModelVariant) -> &'static ModelDescriptor {
    match variant {
        ModelVariant::Htdemucs => &HTDEMUCS,
        ModelVariant::HtdemucsFt => &HTDEMUCS_FT,
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ModelSource {
    ManagedInstall,
    DevelopmentFallback,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ResolvedModelPath {
    pub path: PathBuf,
    pub source: ModelSource,
}

/// Result of locating a managed + optional dev ONNX install for a pinned SHA-256.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ModelInstallationResolution {
    /// Checksum matches at managed or dev path.
    Ready(ResolvedModelPath),
    /// A file exists at the managed install path but its digest does not match the
    /// pinned release. The file is kept so the user can delete it from Settings.
    LegacyManaged(PathBuf),
    /// No verified install at either location.
    Absent,
}

pub fn managed_model_path(app_data_dir: &Path) -> PathBuf {
    managed_model_path_for(app_data_dir, &HTDEMUCS)
}

pub fn managed_model_path_for(app_data_dir: &Path, descriptor: &ModelDescriptor) -> PathBuf {
    app_data_dir.join("models").join(descriptor.filename)
}

/// Check if a specific model variant is present and valid on disk.
pub fn is_model_available(app_data_dir: &Path, variant: ModelVariant) -> bool {
    let descriptor = descriptor_for(variant);
    let path = managed_model_path_for(app_data_dir, descriptor);
    path.exists() && verify_file_checksum(&path, descriptor.sha256).unwrap_or(false)
}

/// Get the file size of a model variant if it exists on disk.
pub fn model_file_size(app_data_dir: &Path, variant: ModelVariant) -> Option<u64> {
    let descriptor = descriptor_for(variant);
    let path = managed_model_path_for(app_data_dir, descriptor);
    fs::metadata(&path).ok().map(|m| m.len())
}

/// True when a managed file exists but does not match the pinned checksum.
pub fn legacy_managed_install_present(app_data_dir: &Path, variant: ModelVariant) -> bool {
    let descriptor = descriptor_for(variant);
    let path = managed_model_path_for(app_data_dir, descriptor);
    path.exists()
        && !verify_file_checksum(&path, descriptor.sha256).unwrap_or(false)
}

/// Delete a model variant from disk.
pub fn delete_model_file(app_data_dir: &Path, variant: ModelVariant) -> Result<()> {
    let descriptor = descriptor_for(variant);
    let path = managed_model_path_for(app_data_dir, descriptor);
    if path.exists() {
        fs::remove_file(&path)
            .with_context(|| format!("failed to delete model file {}", path.display()))?;
    }
    Ok(())
}

pub fn resolve_model_installation(
    managed_path: &Path,
    dev_path: &Path,
    expected_sha256: &str,
) -> Result<ModelInstallationResolution> {
    let managed_invalid = if managed_path.exists() {
        let ok = verify_file_checksum(managed_path, expected_sha256).with_context(|| {
            format!(
                "failed to verify managed model {}",
                managed_path.display()
            )
        })?;
        if ok {
            return Ok(ModelInstallationResolution::Ready(ResolvedModelPath {
                path: managed_path.to_path_buf(),
                source: ModelSource::ManagedInstall,
            }));
        }
        true
    } else {
        false
    };

    if dev_path.exists()
        && verify_file_checksum(dev_path, expected_sha256)
            .with_context(|| format!("failed to verify development model {}", dev_path.display()))?
    {
        return Ok(ModelInstallationResolution::Ready(ResolvedModelPath {
            path: dev_path.to_path_buf(),
            source: ModelSource::DevelopmentFallback,
        }));
    }

    if managed_invalid {
        return Ok(ModelInstallationResolution::LegacyManaged(
            managed_path.to_path_buf(),
        ));
    }

    Ok(ModelInstallationResolution::Absent)
}

pub fn resolve_existing_model_path(
    managed_path: &Path,
    dev_path: &Path,
    expected_sha256: &str,
) -> Result<Option<ResolvedModelPath>> {
    Ok(
        match resolve_model_installation(managed_path, dev_path, expected_sha256)? {
            ModelInstallationResolution::Ready(path) => Some(path),
            ModelInstallationResolution::LegacyManaged(_) | ModelInstallationResolution::Absent => {
                None
            }
        },
    )
}

pub fn install_verified_model_bytes(
    destination: &Path,
    payload: &[u8],
    expected_sha256: &str,
) -> Result<()> {
    let actual_sha256 = sha256_hex(payload);
    if actual_sha256 != expected_sha256 {
        bail!(
            "downloaded model checksum mismatch: expected {expected_sha256}, got {actual_sha256}"
        );
    }

    let parent = destination.parent().with_context(|| {
        format!(
            "model destination {} is missing a parent directory",
            destination.display()
        )
    })?;
    fs::create_dir_all(parent).with_context(|| {
        format!(
            "failed to create model destination directory {}",
            parent.display()
        )
    })?;

    let temp_path = temporary_download_path(destination);
    fs::write(&temp_path, payload).with_context(|| {
        format!(
            "failed to write temporary model download {}",
            temp_path.display()
        )
    })?;
    fs::rename(&temp_path, destination).with_context(|| {
        format!(
            "failed to move verified model from {} to {}",
            temp_path.display(),
            destination.display()
        )
    })?;

    Ok(())
}

pub fn download_and_install_model(
    destination: &Path,
    download_url: &str,
    expected_sha256: &str,
    mut progress: impl FnMut(u64, Option<u64>),
) -> Result<()> {
    let client = Client::builder()
        .build()
        .context("failed to build model download client")?;
    let mut response = client
        .get(download_url)
        .send()
        .and_then(|response| response.error_for_status())
        .with_context(|| format!("failed to download ONNX model from {download_url}"))?;

    let total_bytes = response.content_length();
    let mut last_emit_bytes = 0_u64;
    let mut last_emit_at = Instant::now();
    let emit_interval = Duration::from_millis(150);
    let emit_min_step: u64 = 256 * 1024;

    let mut emit = |downloaded: u64, total: Option<u64>, force: bool| {
        let step_ok = downloaded.saturating_sub(last_emit_bytes) >= emit_min_step;
        let time_ok = last_emit_at.elapsed() >= emit_interval;
        if force || step_ok || time_ok {
            progress(downloaded, total);
            last_emit_bytes = downloaded;
            last_emit_at = Instant::now();
        }
    };

    emit(0, total_bytes, true);

    let mut payload = Vec::new();
    let mut downloaded_bytes = 0_u64;
    let mut buffer = [0_u8; 64 * 1024];

    loop {
        let read = response
            .read(&mut buffer)
            .context("failed while streaming ONNX model download")?;
        if read == 0 {
            break;
        }

        payload.extend_from_slice(&buffer[..read]);
        downloaded_bytes += read as u64;
        emit(downloaded_bytes, total_bytes, false);
    }

    emit(downloaded_bytes, total_bytes, true);

    install_verified_model_bytes(destination, &payload, expected_sha256)
}

fn verify_file_checksum(path: &Path, expected_sha256: &str) -> Result<bool> {
    let bytes =
        fs::read(path).with_context(|| format!("failed to read model file {}", path.display()))?;
    Ok(sha256_hex(&bytes) == expected_sha256)
}

fn sha256_hex(bytes: &[u8]) -> String {
    let mut hasher = Sha256::new();
    hasher.update(bytes);
    format!("{:x}", hasher.finalize())
}

fn temporary_download_path(destination: &Path) -> PathBuf {
    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_nanos();

    destination.with_extension(format!("download.{timestamp}.tmp"))
}
