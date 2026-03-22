# src-tauri — Rust Backend

Tauri application backend. Handles all heavy processing:

| Module       | Responsibility                                           |
| ------------ | -------------------------------------------------------- |
| `audio/`     | Audio decoding (symphonia) and playback (cpal)           |
| `separator/` | AI stem separation — Demucs v4 via ONNX Runtime          |
| `lyrics/`    | Synced lyrics fetching (LRCLIB + LrcApi) and LRC parsing |
| `metadata/`  | Audio file tag reading (ID3, Vorbis, FLAC via lofty)     |
| `cache/`     | SQLite database and file-based cache for stems/lyrics    |
| `commands/`  | Tauri IPC command handlers exposed to the frontend       |

## Models

The `models/` directory holds ONNX model files. These are large binaries and are **git-ignored**. Run the setup script to download them before first build.

## Migrations

SQLite schema migrations live in `migrations/`. Applied automatically on app startup.
