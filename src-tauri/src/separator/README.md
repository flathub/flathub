# separator

Stem separation backend built around the embedded Demucs ONNX model.

Current coverage:

- resolve the runtime model from either:
  - `<app_data_dir>/models/htdemucs.onnx`
  - `src-tauri/models/htdemucs.onnx` as the local development fallback
- verify the model file with a pinned SHA-256 checksum
- download the model in the background on first launch when neither location is
  ready yet
- preprocess decoded stereo PCM into the model's fixed input window
- run ORT inference, including zero-filled auxiliary tensors required by the
  model
- extract the final stem output and write named OGG files for `drums`, `bass`,
  `other`, and `vocals`
- mix `drums + bass + other` into a normalized accompaniment OGG
- cache completed stems and expose separation progress through Tauri events

The runtime bootstrap contract is documented in
`docs/references/contracts/phase-6-model-bootstrap-contract.md`.
