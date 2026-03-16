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
│   │   ├── audio/          # Audio decode & playback
│   │   ├── separator/      # AI stem separation (ONNX)
│   │   ├── lyrics/         # Lyrics fetching & parsing
│   │   ├── metadata/       # Audio file tag reading
│   │   ├── cache/          # Cache management (SQLite + fs)
│   │   └── commands/       # Tauri IPC command handlers
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
│   ├── stores/             # State management (Zustand)
│   │   └── queue-store.ts  # Playback queue state
│   ├── lib/                # Shared utilities
│   │   ├── tauri.ts        # Tauri IPC helpers
│   │   ├── errors.ts       # Error handling utilities
│   │   └── format.ts       # Formatting helpers
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

All heavy lifting happens here: audio decoding, AI inference, lyrics fetching, and caching. The frontend communicates with this layer through Tauri's IPC command system.

### `src/` — React Frontend

The UI layer. Renders the karaoke experience: song library, lyrics display with synchronized highlighting, and playback controls. Stays thin — no audio processing or AI logic.

### `docs/` — Documentation

Architecture decisions, project structure, and development guides. Kept in the repo so documentation evolves with the code.

### `src-tauri/models/` — AI Models

ONNX model files for Demucs v4. These are large binary files (~80 MB) and are **not checked into git**. A setup script downloads them on first build.
