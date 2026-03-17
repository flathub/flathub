# Project Structure

```text
OpenKara/
├── AGENTS.md                  # Repository-specific agent and verification rules
├── ARCHITECTURE.md            # Root entry point to the canonical architecture docs
├── docs/
│   ├── README.md              # Documentation hub
│   ├── design-docs/           # Architecture, roadmap, release, and delivery docs
│   │   ├── index.md
│   │   ├── architecture.md
│   │   ├── core-beliefs.md
│   │   ├── project-structure.md
│   │   └── performance/
│   ├── exec-plans/            # Actionable and historical implementation plans
│   │   ├── active/
│   │   ├── completed/
│   │   └── tech-debt-tracker.md
│   ├── generated/             # Generated or migration-derived reference docs
│   │   └── db-schema.md
│   ├── product-specs/         # Product-facing behavior and experience specs
│   │   ├── index.md
│   │   └── new-user-onboarding.md
│   ├── references/            # Frozen contracts and other reference material
│   │   └── contracts/
│   └── site/                  # GitHub Pages public microsite
├── public/                    # Static frontend assets
├── src/                       # React frontend
│   ├── components/
│   │   ├── Bootstrap/         # Model bootstrap UI
│   │   ├── Cdg/               # CD+G rendering widgets
│   │   ├── Layout/            # App shell, sidebar, toolbar, toasts
│   │   ├── Library/           # Song list, import, metadata editing
│   │   ├── Lyrics/            # Lyrics display and editing flows
│   │   ├── Player/            # Playback controls, queue, fullscreen player
│   │   └── Settings/          # Settings and setup dialogs
│   ├── hooks/                 # React runtime hooks for sync and platform events
│   ├── lib/                   # Shared frontend helpers and Tauri IPC wrappers
│   ├── locales/               # i18n resource bundles
│   ├── stores/                # Zustand state stores
│   ├── styles/                # Global CSS
│   └── types/                 # Shared TypeScript types
├── src-tauri/                 # Rust backend (Tauri)
│   ├── migrations/            # SQLite schema migrations
│   ├── models/                # ONNX model files for local development
│   ├── src/
│   │   ├── audio/             # Decode, resample, output, and encode helpers
│   │   ├── cache/             # SQLite + filesystem cache logic
│   │   ├── cdg/               # CD+G parsing and rendering state
│   │   ├── commands/          # Tauri command handlers and error mapping
│   │   ├── library/           # Library models and library-root utilities
│   │   ├── lyrics/            # Lyrics lookup, parsing, and fetch helpers
│   │   ├── metadata/          # Audio tag extraction
│   │   └── separator/         # Model bootstrap, inference, and stem mixing
│   └── tauri.conf.json        # Tauri bundle/runtime configuration
├── README.md                  # English project overview
├── README_CN.md               # Chinese project overview
└── package.json               # Frontend scripts and dependencies
```

## Directory Responsibilities

### `docs/`

The documentation tree is split by purpose: design docs explain the system, execution plans track work, generated docs summarize machine-derived state, product specs describe user-facing behavior, and references hold frozen contracts.

### `src/`

The frontend renders the library, lyrics, player, setup, and fullscreen experiences. It should stay focused on presentation, orchestration, and user interactions while delegating audio processing and persistence to the backend.

### `src-tauri/`

The backend owns file access, metadata extraction, lyrics lookup, CD+G parsing, separation, cache persistence, and Tauri IPC. This is where the expensive or platform-specific work lives.

### `src-tauri/models/`

Model files are large binaries and are not committed. Local development or first-run bootstrap places them here or inside the app data directory depending on runtime context.
