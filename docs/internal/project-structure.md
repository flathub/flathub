# Project Structure

```
OpenKara/
├── docs/                   # Project documentation
│   ├── architecture.md     # System architecture & tech stack
│   └── project-structure.md# This file
│
├── src-tauri/              # Rust backend (Tauri)
│   ├── Cargo.toml
│   ├── src/
│   │   ├── main.rs         # Tauri app entry point
│   │   ├── audio/          # Audio decode, mixing, and low-level playback primitives
│   │   ├── macos/          # AppKit/Objective-C bridges used only on macOS
│   │   ├── separator/      # AI stem separation (ONNX)
│   │   ├── lyrics/         # Lyrics fetching & parsing
│   │   ├── metadata/       # Audio file tag reading
│   │   ├── cache/          # Cache management (SQLite + fs)
│   │   ├── commands/       # Thin Tauri IPC command handlers
│   │   ├── window_shell.rs # Cross-platform window shell profile + macOS setup
│   │   └── services/       # Backend orchestration/service boundaries
│   ├── models/             # ONNX model files (git-ignored)
│   └── migrations/         # SQLite schema migrations
│
├── src/                    # React frontend
│   ├── main.tsx            # App entry point
│   ├── App.tsx
│   ├── components/         # React components
│   │   ├── Player/         # Karaoke player & controls
│   │   │   ├── QueueButton.tsx      # Toggle queue panel
│   │   │   └── QueuePanel.tsx       # Drag-to-reorder queue UI
│   │   ├── Library/        # Song library & import
│   │   │   ├── SongPropertiesDialog.tsx  # Audio file properties
│   │   │   └── SongEditDialog.tsx        # Song metadata editing
│   │   ├── Lyrics/         # Synced lyrics display
│   │   │   └── LyricsEditDialog.tsx      # Manual lyrics input
│   │   └── Settings/       # App settings UI
│   ├── hooks/              # Custom React hooks
│   ├── runtime/            # App/runtime orchestration boundaries
│   ├── stores/             # State management (Zustand)
│   │   ├── player-workflows.ts # Shared playback workflow helpers
│   │   ├── queue-store.ts      # Playback queue state
│   │   └── settings-store.ts   # App settings single source of truth
│   ├── lib/                # Shared utilities
│   │   ├── tauri.ts        # Tauri IPC helpers
│   │   ├── errors.ts       # Error handling utilities
│   │   ├── format.ts       # Formatting helpers
│   │   └── window-shell.ts # Frontend normalization for native/legacy shell profiles
│   ├── types/              # TypeScript type definitions
│   └── styles/             # Global styles
│
├── public/                 # Static assets
├── README.md
├── README_CN.md
├── package.json
├── tsconfig.json
├── vite.config.ts
└── .gitignore
```

## Directory Responsibilities

### `src-tauri/` — Rust Backend

All heavy lifting happens here: audio decoding, AI inference, lyrics fetching, caching, and platform-specific shell setup. The frontend communicates with this layer through Tauri's IPC command system. `commands/` now stays thin, while `services/` owns orchestration such as playback startup, CDG state loading, and separation worker coordination. `window_shell.rs` owns the cross-platform shell profile that keeps macOS-native chrome isolated from Windows/Linux behavior.

### `src/` — React Frontend

The UI layer. Renders the karaoke experience: song library, lyrics display with synchronized highlighting, playback controls, and the shared desktop shell. Runtime side effects are centralized under `src/runtime/`, and persisted app settings are unified in `settings-store` so components no longer hydrate settings independently. `src/lib/window-shell.ts` normalizes the Rust shell snapshot into frontend tokens so the macOS treatment can evolve without forking Windows/Linux layout code.

### `docs/` — Documentation

Architecture decisions, project structure, and development guides. Kept in the repo so documentation evolves with the code.

### `src-tauri/models/` — Development Model Cache

Local ONNX model cache for Demucs v4 during development and deterministic tests. These binaries are **not checked into git**. Runtime installs use the app data directory instead; `scripts/setup.sh` only prewarms this repo path for local workflows.
