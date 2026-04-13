# separator

Stem separation backend built around the embedded Demucs ONNX model.

Current coverage:

- resolve the runtime model from either:
  - `<app_data_dir>/models/<active-variant>.onnx`
  - `src-tauri/models/<active-variant>.onnx` as the local development fallback
- resolve the ONNX Runtime shared library from either:
  - bundled app resources / macOS Frameworks
  - `src-tauri/generated/onnxruntime/` as the local development and CI staging
- prepare macOS release bundles with the target-matching `arm64` or `x86_64`
  ORT dylib instead of shipping one universal2 runtime in every app
- verify the model file with a pinned SHA-256 checksum
- download the model in the background on first launch when neither location is
  ready yet
- explicitly initialize `ort` against the staged ONNX Runtime 1.23.2 shared
  library before any session builder is used
- read `openkara.model_cache_key` / `openkara.optimized_by` from ONNX
  `metadata_props` so session reuse, CoreML compiled cache invalidation, and
  runtime graph optimization stay aligned with the shipped model bytes
- preprocess decoded stereo PCM into the model's fixed input window
- run ORT inference, including zero-filled auxiliary tensors required by the
  model
- extract the final stem output and write named OGG files for `drums`, `bass`,
  `other`, and `vocals`
- mix `drums + bass + other` into a normalized accompaniment OGG
- cache completed stems and expose separation progress through Tauri events

The runtime bootstrap contract is documented in
`docs/references/contracts/phase-6-model-bootstrap-contract.md`.
