use anyhow::{Context, Result};
use ort::value::TensorElementType;
use std::path::{Path, PathBuf};

pub const EMBEDDED_MODEL_FILENAME: &str = "htdemucs.onnx";

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

pub fn load_from_path(path: &Path) -> Result<LoadedModel> {
    let model_path = path.to_path_buf();
    let num_threads = std::thread::available_parallelism()
        .map(|n| n.get().min(8))
        .unwrap_or(4);

    let session = ort::session::Session::builder()
        .context("failed to create ONNX session builder")?
        .with_intra_threads(num_threads)
        .map_err(|e| anyhow::anyhow!("failed to set intra-op thread count: {e}"))?
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
