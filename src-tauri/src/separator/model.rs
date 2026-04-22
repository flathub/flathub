use crate::config::ExecutionProviderPreference;
use anyhow::{Context, Result};
use ort::{session::builder::GraphOptimizationLevel, tensor::TensorElementType};
use std::{
    fs,
    path::{Path, PathBuf},
    sync::{Mutex, OnceLock},
    time::Instant,
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

const MODEL_CACHE_KEY_METADATA: &str = "openkara.model_cache_key";
const OPTIMIZED_BY_METADATA: &str = "openkara.optimized_by";
const ONNXRUNTIME_OPTIMIZED_BY_VALUE: &str = "onnxruntime";

#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub(crate) struct ModelRuntimeMetadata {
    pub model_cache_key: Option<String>,
    pub optimized_by: Option<String>,
}

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

pub(crate) fn read_model_runtime_metadata(path: &Path) -> Result<ModelRuntimeMetadata> {
    let bytes =
        fs::read(path).with_context(|| format!("failed to read model file {}", path.display()))?;
    Ok(parse_model_runtime_metadata(&bytes))
}

pub(crate) fn session_cache_key(
    model_path: &Path,
    provider: ExecutionProviderPreference,
    metadata: &ModelRuntimeMetadata,
) -> String {
    match metadata.model_cache_key.as_deref() {
        Some(model_cache_key) => format!(
            "{}::{}::{}",
            model_path.display(),
            provider.as_str(),
            model_cache_key
        ),
        None => format!("{}::{}", model_path.display(), provider.as_str()),
    }
}

pub fn load_from_path(
    path: &Path,
    ep_preference: ExecutionProviderPreference,
) -> Result<LoadedModel> {
    eprintln!(
        "Attempting ONNX session load for {} via {}",
        path.display(),
        provider_diagnostic_summary(ep_preference)
    );

    let provider_chain = execution_provider_chain(ep_preference);
    let mut last_error = None;

    for (index, provider) in provider_chain.iter().copied().enumerate() {
        match load_with_ep(path, provider) {
            Ok(model) => {
                if index > 0 {
                    eprintln!(
                        "Recovered ONNX session load by falling back to {} for {}",
                        provider.as_str(),
                        path.display()
                    );
                }
                return Ok(model);
            }
            Err(error) => {
                if index + 1 < provider_chain.len() {
                    eprintln!(
                        "ONNX session load failed with {} for {}: {error:#}",
                        provider.as_str(),
                        path.display()
                    );
                }
                last_error = Some(error);
            }
        }
    }

    Err(last_error.expect("provider chain should contain at least one provider"))
}

pub fn provider_diagnostic_summary(preference: ExecutionProviderPreference) -> String {
    execution_provider_chain(preference)
        .into_iter()
        .map(|provider| provider.as_str())
        .collect::<Vec<_>>()
        .join(" -> ")
}

fn load_with_ep(path: &Path, ep_preference: ExecutionProviderPreference) -> Result<LoadedModel> {
    ensure_runtime_loaded(None)?;
    let runtime_metadata = read_model_runtime_metadata(path)?;

    let model_path = path.to_path_buf();
    let num_threads = std::thread::available_parallelism()
        .map(|n| n.get().min(8))
        .unwrap_or(4);

    let mut builder =
        ort::session::Session::builder().context("failed to create ONNX session builder")?;

    // `intra_threads` controls ORT CPU EP intra-op parallelism for operators
    // that XNNPACK does not handle (for example, some LSTM nodes).
    builder = builder
        .with_intra_threads(num_threads)
        .map_err(|e| anyhow::anyhow!("failed to set intra-op thread count: {e}"))?;

    // XNNPACK has its own internal worker pool. When XNNPACK owns conv/matmul
    // operators, ORT intra-op spinning can compete for CPU time; disable
    // spinning so the OS scheduler can arbitrate cores fairly.
    if matches!(ep_preference, ExecutionProviderPreference::Xnnpack) {
        builder = builder
            .with_intra_op_spinning(false)
            .map_err(|e| anyhow::anyhow!("failed to disable intra-op spinning: {e}"))?;
    }

    builder = builder
        .with_optimization_level(graph_optimization_level_for(&runtime_metadata))
        .map_err(|e| anyhow::anyhow!("failed to set graph optimization level: {e}"))?;

    // Register execution providers. ORT falls back to CPU automatically if the
    // requested EP is unavailable, but we log the attempt so users can diagnose
    // performance issues.
    let ep_list = build_execution_provider_list(ep_preference, num_threads);
    if !ep_list.is_empty() {
        builder = builder
            .with_execution_providers(ep_list)
            .map_err(|e| anyhow::anyhow!("failed to configure execution providers: {e}"))?;
    }

    eprintln!(
        "Committing ONNX session for {} (provider preference: {})",
        path.display(),
        ep_preference.as_str()
    );
    let commit_start = Instant::now();
    let session = builder
        .commit_from_file(path)
        .with_context(|| format!("failed to load ONNX model from {}", path.display()))?;
    eprintln!(
        "Committed ONNX session for {} in {:?}",
        path.display(),
        commit_start.elapsed()
    );

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
    num_threads: usize,
) -> Vec<ort::ep::ExecutionProviderDispatch> {
    use ort::ep;
    use std::num::NonZeroUsize;

    match preference {
        // Empty list means ORT uses the built-in CPU EP.
        ExecutionProviderPreference::Cpu => vec![],
        // Keep XNNPACK worker count aligned with ORT intra-op threads to avoid
        // oversubscription. Unsupported operators fall back to ORT CPU EP.
        ExecutionProviderPreference::Xnnpack => vec![ep::XNNPACK::default()
            .with_intra_op_num_threads(
                NonZeroUsize::new(num_threads).expect("num_threads is non-zero"),
            )
            .build()],
        ExecutionProviderPreference::DirectMl => vec![ep::DirectML::default().build()],
    }
}

fn execution_provider_chain(
    preference: ExecutionProviderPreference,
) -> Vec<ExecutionProviderPreference> {
    match preference {
        // If XNNPACK session creation fails, drop to bare CPU.
        ExecutionProviderPreference::Xnnpack => vec![
            ExecutionProviderPreference::Xnnpack,
            ExecutionProviderPreference::Cpu,
        ],
        // If DirectML fails, retry with XNNPACK SIMD before bare CPU.
        ExecutionProviderPreference::DirectMl => vec![
            ExecutionProviderPreference::DirectMl,
            ExecutionProviderPreference::Xnnpack,
            ExecutionProviderPreference::Cpu,
        ],
        resolved => vec![resolved],
    }
}

fn parse_model_runtime_metadata(bytes: &[u8]) -> ModelRuntimeMetadata {
    // We only need two custom ONNX metadata properties at runtime. Reading just
    // `metadata_props` avoids pulling a full protobuf dependency into the app.
    let mut metadata = ModelRuntimeMetadata::default();
    let mut cursor = 0;

    while let Some(tag) = decode_varint(bytes, &mut cursor) {
        let field_number = tag >> 3;
        let wire_type = (tag & 0x07) as u8;

        if field_number == 14 && wire_type == 2 {
            let Some(entry_bytes) = read_length_delimited(bytes, &mut cursor) else {
                break;
            };
            let Some((key, value)) = parse_string_string_entry(entry_bytes) else {
                continue;
            };

            match key.as_str() {
                MODEL_CACHE_KEY_METADATA => metadata.model_cache_key = Some(value),
                OPTIMIZED_BY_METADATA => metadata.optimized_by = Some(value),
                _ => {}
            }
            continue;
        }

        if !skip_field(bytes, &mut cursor, wire_type) {
            break;
        }
    }

    metadata
}

fn graph_optimization_level_for(metadata: &ModelRuntimeMetadata) -> GraphOptimizationLevel {
    if metadata.optimized_by.as_deref() == Some(ONNXRUNTIME_OPTIMIZED_BY_VALUE) {
        GraphOptimizationLevel::Disable
    } else {
        GraphOptimizationLevel::Level3
    }
}

fn parse_string_string_entry(bytes: &[u8]) -> Option<(String, String)> {
    let mut cursor = 0;
    let mut key = None;
    let mut value = None;

    while let Some(tag) = decode_varint(bytes, &mut cursor) {
        let field_number = tag >> 3;
        let wire_type = (tag & 0x07) as u8;

        match (field_number, wire_type) {
            (1, 2) => {
                let entry = read_length_delimited(bytes, &mut cursor)?;
                key = Some(std::str::from_utf8(entry).ok()?.to_owned());
            }
            (2, 2) => {
                let entry = read_length_delimited(bytes, &mut cursor)?;
                value = Some(std::str::from_utf8(entry).ok()?.to_owned());
            }
            _ => {
                if !skip_field(bytes, &mut cursor, wire_type) {
                    return None;
                }
            }
        }
    }

    Some((key?, value?))
}

fn decode_varint(bytes: &[u8], cursor: &mut usize) -> Option<u64> {
    let mut value = 0_u64;
    let mut shift = 0_u32;

    while *cursor < bytes.len() && shift < 64 {
        let byte = bytes[*cursor];
        *cursor += 1;
        value |= u64::from(byte & 0x7f) << shift;
        if byte & 0x80 == 0 {
            return Some(value);
        }
        shift += 7;
    }

    None
}

fn read_length_delimited<'a>(bytes: &'a [u8], cursor: &mut usize) -> Option<&'a [u8]> {
    let length = decode_varint(bytes, cursor)? as usize;
    let end = cursor.checked_add(length)?;
    if end > bytes.len() {
        return None;
    }

    let slice = &bytes[*cursor..end];
    *cursor = end;
    Some(slice)
}

fn skip_field(bytes: &[u8], cursor: &mut usize, wire_type: u8) -> bool {
    match wire_type {
        0 => decode_varint(bytes, cursor).is_some(),
        1 => advance_cursor(bytes, cursor, 8),
        2 => read_length_delimited(bytes, cursor).is_some(),
        5 => advance_cursor(bytes, cursor, 4),
        _ => false,
    }
}

fn advance_cursor(bytes: &[u8], cursor: &mut usize, length: usize) -> bool {
    let Some(end) = cursor.checked_add(length) else {
        return false;
    };
    if end > bytes.len() {
        return false;
    }

    *cursor = end;
    true
}

#[cfg(test)]
fn encode_varint(mut value: u64, bytes: &mut Vec<u8>) {
    loop {
        let mut byte = (value & 0x7f) as u8;
        value >>= 7;
        if value != 0 {
            byte |= 0x80;
        }
        bytes.push(byte);
        if value == 0 {
            break;
        }
    }
}

#[cfg(test)]
fn encode_length_delimited(payload: &[u8], bytes: &mut Vec<u8>) {
    encode_varint(payload.len() as u64, bytes);
    bytes.extend_from_slice(payload);
}

#[cfg(test)]
mod tests {
    use super::*;
    use ort::session::builder::GraphOptimizationLevel;

    fn metadata_entry_bytes(key: &str, value: &str) -> Vec<u8> {
        let mut entry = Vec::new();
        encode_varint((1_u64 << 3) | 2, &mut entry);
        encode_length_delimited(key.as_bytes(), &mut entry);
        encode_varint((2_u64 << 3) | 2, &mut entry);
        encode_length_delimited(value.as_bytes(), &mut entry);
        entry
    }

    fn model_with_metadata(entries: &[(&str, &str)]) -> Vec<u8> {
        let mut bytes = Vec::new();
        for (key, value) in entries {
            let entry = metadata_entry_bytes(key, value);
            encode_varint((14_u64 << 3) | 2, &mut bytes);
            encode_length_delimited(&entry, &mut bytes);
        }
        bytes
    }

    #[test]
    fn provider_chain_keeps_xnnpack_cpu_fallback() {
        assert_eq!(
            execution_provider_chain(ExecutionProviderPreference::Xnnpack),
            vec![
                ExecutionProviderPreference::Xnnpack,
                ExecutionProviderPreference::Cpu,
            ]
        );
    }

    #[test]
    fn provider_chain_includes_xnnpack_between_directml_and_cpu() {
        assert_eq!(
            execution_provider_chain(ExecutionProviderPreference::DirectMl),
            vec![
                ExecutionProviderPreference::DirectMl,
                ExecutionProviderPreference::Xnnpack,
                ExecutionProviderPreference::Cpu,
            ]
        );
    }

    #[test]
    fn provider_chain_keeps_cpu_only_when_requested() {
        assert_eq!(
            execution_provider_chain(ExecutionProviderPreference::Cpu),
            vec![ExecutionProviderPreference::Cpu]
        );
    }

    #[test]
    fn parses_openkara_runtime_metadata_from_model_bytes() {
        let metadata = parse_model_runtime_metadata(&model_with_metadata(&[
            ("openkara.model_cache_key", "cache-key-123"),
            ("openkara.optimized_by", "onnxruntime"),
        ]));

        assert_eq!(metadata.model_cache_key.as_deref(), Some("cache-key-123"));
        assert_eq!(metadata.optimized_by.as_deref(), Some("onnxruntime"));
    }

    #[test]
    fn reads_openkara_runtime_metadata_from_downloaded_model_file() {
        let metadata =
            read_model_runtime_metadata(&default_model_path()).expect("model metadata should load");

        assert!(metadata.model_cache_key.is_some());
        assert_eq!(
            metadata.optimized_by.as_deref(),
            Some(ONNXRUNTIME_OPTIMIZED_BY_VALUE)
        );
    }

    #[test]
    fn optimized_model_metadata_disables_graph_optimization() {
        let metadata = ModelRuntimeMetadata {
            model_cache_key: Some("cache-key-123".to_owned()),
            optimized_by: Some("onnxruntime".to_owned()),
        };

        assert_eq!(
            graph_optimization_level_for(&metadata),
            GraphOptimizationLevel::Disable
        );
    }

    #[test]
    fn session_cache_key_includes_model_cache_key_when_present() {
        let model_path = Path::new("/tmp/models/htdemucs.onnx");
        let metadata = ModelRuntimeMetadata {
            model_cache_key: Some("cache-key-123".to_owned()),
            optimized_by: None,
        };

        assert_eq!(
            session_cache_key(model_path, ExecutionProviderPreference::Xnnpack, &metadata),
            "/tmp/models/htdemucs.onnx::xnnpack::cache-key-123"
        );
    }
}
