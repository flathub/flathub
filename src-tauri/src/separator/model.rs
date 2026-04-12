use crate::config::ExecutionProviderPreference;
use anyhow::{Context, Result};
use ort::tensor::TensorElementType;
use std::{
    path::{Path, PathBuf},
    sync::{Mutex, OnceLock},
};

pub const EMBEDDED_MODEL_FILENAME: &str = "htdemucs.onnx";
pub const ORT_RUNTIME_VERSION: &str = "1.23.2";
pub const ORT_RUNTIME_STAGING_DIR: &str = "generated/onnxruntime";

#[cfg(target_os = "windows")]
pub const ORT_RUNTIME_FILENAME: &str = "onnxruntime.dll";
#[cfg(target_os = "linux")]
pub const ORT_RUNTIME_FILENAME: &str = "libonnxruntime.so";
#[cfg(target_vendor = "apple")]
pub const ORT_RUNTIME_FILENAME: &str = "libonnxruntime.dylib";

static ORT_RUNTIME_PATH: OnceLock<PathBuf> = OnceLock::new();
static ORT_RUNTIME_INIT_LOCK: Mutex<()> = Mutex::new(());

pub struct LoadedModel {
    pub model_path: PathBuf,
    pub inputs: Vec<String>,
    pub outputs: Vec<String>,
    pub input_shape: Vec<i64>,
    pub input_tensor_type: TensorElementType,
    pub(crate) session: ort::session::Session,
}

impl std::fmt::Debug for LoadedModel {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LoadedModel")
            .field("model_path", &self.model_path)
            .field("inputs", &self.inputs)
            .field("outputs", &self.outputs)
            .field("input_shape", &self.input_shape)
            .field("input_tensor_type", &self.input_tensor_type)
            .finish_non_exhaustive()
    }
}

pub fn default_model_path_for_filename(filename: &str) -> PathBuf {
    // Dev builds may keep multiple local model variants under `src-tauri/models/`.
    // Callers must resolve by filename instead of assuming the standard model.
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("models")
        .join(filename)
}

pub fn default_model_path() -> PathBuf {
    default_model_path_for_filename(EMBEDDED_MODEL_FILENAME)
}

pub fn default_runtime_library_path() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join(ORT_RUNTIME_STAGING_DIR)
        .join(ORT_RUNTIME_FILENAME)
}

fn bundled_runtime_library_path(resource_dir: &Path) -> PathBuf {
    resource_dir.join("onnxruntime").join(ORT_RUNTIME_FILENAME)
}

#[cfg(target_vendor = "apple")]
fn bundled_framework_runtime_library_path(resource_dir: &Path) -> Option<PathBuf> {
    resource_dir
        .parent()
        .map(|contents_dir| contents_dir.join("Frameworks").join(ORT_RUNTIME_FILENAME))
}

fn resolve_runtime_library_path_with_staging(
    resource_dir: Option<&Path>,
    staged_path: &Path,
) -> Result<PathBuf> {
    if let Some(resource_dir) = resource_dir {
        let bundled_path = bundled_runtime_library_path(resource_dir);
        if bundled_path.is_file() {
            return Ok(bundled_path);
        }

        #[cfg(target_vendor = "apple")]
        if let Some(framework_path) = bundled_framework_runtime_library_path(resource_dir) {
            if framework_path.is_file() {
                return Ok(framework_path);
            }
        }
    }

    if staged_path.is_file() {
        return Ok(staged_path.to_path_buf());
    }

    Err(anyhow::anyhow!(
        "missing ONNX Runtime shared library {}; run ./scripts/setup.sh or node scripts/prepare-onnx-runtime.mjs",
        staged_path.display()
    ))
}

pub fn resolve_runtime_library_path(resource_dir: Option<&Path>) -> Result<PathBuf> {
    resolve_runtime_library_path_with_staging(resource_dir, &default_runtime_library_path())
}

pub fn ensure_runtime_loaded(resource_dir: Option<&Path>) -> Result<&'static Path> {
    if let Some(path) = ORT_RUNTIME_PATH.get() {
        return Ok(path.as_path());
    }

    let _init_guard = ORT_RUNTIME_INIT_LOCK
        .lock()
        .map_err(|_| anyhow::anyhow!("onnx runtime initialization lock was poisoned"))?;
    if let Some(path) = ORT_RUNTIME_PATH.get() {
        return Ok(path.as_path());
    }

    let runtime_path = resolve_runtime_library_path(resource_dir)?;
    let committed = ort::init_from(&runtime_path)?
        .with_name("openkara")
        .commit();
    anyhow::ensure!(
        committed,
        "failed to initialize ONNX Runtime from {} before another ORT environment was configured",
        runtime_path.display()
    );

    // The process-global ORT environment can only be configured once. We persist the
    // exact loaded library path so every later session uses the same runtime contract.
    let _ = ORT_RUNTIME_PATH.set(runtime_path);
    Ok(ORT_RUNTIME_PATH
        .get()
        .expect("runtime path should be stored after successful initialization")
        .as_path())
}

pub fn resolve_runtime_library_path_for_tests(
    resource_dir: Option<&Path>,
    staged_path: &Path,
) -> Result<PathBuf> {
    resolve_runtime_library_path_with_staging(resource_dir, staged_path)
}

pub fn load_from_path(
    path: &Path,
    ep_preference: ExecutionProviderPreference,
) -> Result<LoadedModel> {
    match load_with_ep(path, ep_preference) {
        Ok(model) => Ok(model),
        Err(accel_error) if ep_preference != ExecutionProviderPreference::Cpu => {
            // Hardware acceleration failed — fall back to CPU so the user
            // can still separate stems. Log the original error for diagnostics.
            eprintln!(
                "Hardware-accelerated ONNX session failed ({}), falling back to CPU: {accel_error:#}",
                ep_preference.as_str()
            );
            load_with_ep(path, ExecutionProviderPreference::Cpu)
        }
        Err(e) => Err(e),
    }
}

fn load_with_ep(
    path: &Path,
    ep_preference: ExecutionProviderPreference,
) -> Result<LoadedModel> {
    ensure_runtime_loaded(None)?;

    let model_path = path.to_path_buf();
    let num_threads = std::thread::available_parallelism()
        .map(|n| n.get().min(8))
        .unwrap_or(4);

    let mut builder = ort::session::Session::builder()
        .context("failed to create ONNX session builder")?;

    builder = builder
        .with_intra_threads(num_threads)
        .map_err(|e| anyhow::anyhow!("failed to set intra-op thread count: {e}"))?;

    // Register platform-specific execution providers. ORT falls back to CPU
    // automatically if the requested EP is unavailable, but we log the attempt
    // so users can diagnose performance issues.
    let ep_list = build_execution_provider_list(ep_preference);
    if !ep_list.is_empty() {
        builder = builder
            .with_execution_providers(ep_list)
            .map_err(|e| anyhow::anyhow!("failed to configure execution providers: {e}"))?;
    }

    let session = builder
        .commit_from_file(path)
        .with_context(|| format!("failed to load ONNX model from {}", path.display()))?;

    let inputs = session
        .inputs()
        .iter()
        .map(|input| input.name().to_owned())
        .collect();
    let outputs = session
        .outputs()
        .iter()
        .map(|output| output.name().to_owned())
        .collect();
    let input_spec = session
        .inputs()
        .first()
        .context("model did not expose any inputs")?;
    let input_shape = input_spec
        .dtype()
        .tensor_shape()
        .context("model input is not a tensor")?
        .iter()
        .copied()
        .collect();
    let input_tensor_type = input_spec
        .dtype()
        .tensor_type()
        .context("model input tensor type is missing")?;

    Ok(LoadedModel {
        model_path,
        inputs,
        outputs,
        input_shape,
        input_tensor_type,
        session,
    })
}

fn build_execution_provider_list(
    preference: ExecutionProviderPreference,
) -> Vec<ort::ep::ExecutionProviderDispatch> {
    use ort::ep;

    match resolve_ep_for_platform(preference) {
        ExecutionProviderPreference::Cpu => vec![],
        ExecutionProviderPreference::CoreMl => {
            vec![ep::CoreML::default().with_subgraphs(true).build()]
        }
        ExecutionProviderPreference::DirectMl => {
            vec![ep::DirectML::default().build()]
        }
        ExecutionProviderPreference::Auto => {
            // Auto is resolved by resolve_ep_for_platform, so this branch
            // should not be reached. Fall back to CPU as a safety net.
            vec![]
        }
    }
}

/// Resolve `Auto` to a concrete EP based on the current platform.
fn resolve_ep_for_platform(
    preference: ExecutionProviderPreference,
) -> ExecutionProviderPreference {
    if preference != ExecutionProviderPreference::Auto {
        return preference;
    }

    #[cfg(all(target_vendor = "apple", target_arch = "aarch64"))]
    {
        return ExecutionProviderPreference::CoreMl;
    }

    #[cfg(all(target_vendor = "apple", not(target_arch = "aarch64")))]
    {
        return ExecutionProviderPreference::Cpu;
    }

    #[cfg(target_os = "windows")]
    {
        return ExecutionProviderPreference::DirectMl;
    }

    #[cfg(not(any(target_vendor = "apple", target_os = "windows")))]
    {
        return ExecutionProviderPreference::Cpu;
    }
}
