# Scripts

## `setup.sh`

Bootstraps the local Demucs ONNX model required by later separation work.

- **Input:** none
- **Prerequisites:** `curl`, `shasum`
- **Output:** `src-tauri/models/htdemucs.onnx`
- **Success:** downloads the model, verifies SHA-256, and stores it in the models directory
- **Repeat runs:** exit immediately if the existing model already matches the pinned checksum
- **Failure:** exits non-zero with a readable error if a required tool is missing, the download fails, or checksum verification fails

Run it from the repository root:

```bash
./scripts/setup.sh
```

## `run-local-smoke.sh`

Runs a local backend smoke pass against real audio files in a directory and
writes JSON + Markdown reports into an output directory.

- **Input:** optional input directory, defaults to `./test`
- **Prerequisites:** Rust toolchain, local dependencies installed, optional
  model downloaded via `./scripts/setup.sh` if separation should run
- **Output:** `output/local-audio-smoke-report.json`,
  `output/local-audio-smoke-report.md`, and separation cache under
  `output/cache/`
- **Success:** imports supported audio files, profiles playback load/seek, and
  runs separation when a verified model is available
- **Repeat runs:** overwrite the smoke DB/report files while reusing any cached
  stems under the selected output directory
- **Failure:** exits non-zero with readable stderr when the input directory is
  missing, no readable audio files are found, or a backend step fatally fails

Run it from the repository root:

```bash
./scripts/run-local-smoke.sh
```

Optional custom paths:

```bash
./scripts/run-local-smoke.sh ./test ./output
```
