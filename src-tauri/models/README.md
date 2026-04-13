# models

This directory is a local cache for large model binaries used during
development and deterministic tests.

Rules:

- Commit `.gitkeep` only.
- Do **not** commit downloaded `.onnx` files.
- `scripts/setup.sh` may populate this directory for local development and
  tests with the pinned `htdemucs` v2.0.1 release asset. It also stages the matching ONNX Runtime shared library under
  `src-tauri/generated/onnxruntime/`.
- Runtime installs use the app data directory instead of this repo path.
- End-user bundles should not rely on this directory being present.

The current default model filename is `htdemucs.onnx`.
