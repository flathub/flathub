[简体中文](./README_CN.md)

<div align="center">

<img src="./src-tauri/icons/app-icon.png" alt="OpenKara app icon" width="160" height="160" />

# OpenKara

**Turn your music library into a karaoke stage.**

An open-source desktop karaoke app powered by on-device AI stem separation and synced lyrics.

[![CI](https://github.com/thedavidweng/OpenKara/actions/workflows/ci.yml/badge.svg)](https://github.com/thedavidweng/OpenKara/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey)

</div>

---

## Demo

<div align="center">

[![OpenKara demo video](https://img.youtube.com/vi/OznVDmp9igk/maxresdefault.jpg)](https://www.youtube-nocookie.com/embed/OznVDmp9igk)

</div>

---

## Why I Built This

I love singing karaoke at home, but every existing solution has its own set of problems.

The most mature option is probably [Karafun](https://www.karafun.com/) — a paid service that sidesteps copyright by re-recording famous songs. That's neat, but it comes with issues:

1. Their re-recorded instrumentals inevitably sound a little different from the originals
2. Their catalog doesn't always include the niche songs I want to sing
3. I hate subscriptions

Then there's [Apple Music Sing](https://www.apple.com/ca/newsroom/2022/12/apple-introduces-apple-music-sing/), which offers on-device vocal removal for karaoke. Also neat — but Apple Music is yet another subscription, and I hate subscriptions.

To dodge the subscription trap, you could go the more traditional route — something like [OpenKJ](https://github.com/OpenKJ/OpenKJ) for playing CD+G/media+g files. But CD+G files are niche, hard to find, and have to be purchased separately.

That pretty much leaves scouring YouTube for karaoke videos of dubious origin and questionable copyright status. Not exactly a unified experience, and the song I want is missing half the time.

So my no-compromise solution was born: OpenKara uses open-source AI to separate the digital music you already own in unencrypted form — whether it's from CD rips, [Bandcamp](https://bandcamp.com/), [Qobuz](https://www.qobuz.com/), iTunes, or your local library's music service. I know there are plenty of people who, like me, prefer to buy once and own forever. OpenKara turns my existing music library into a karaoke library, so I don't have to pay for KTV, and my catalog is shaped by my own taste — not the mainstream.

## Features

- **Local Audio Import** — Use music you already own. No subscriptions, no repurchases.
- **AI Stem Separation** — Separate vocals and accompaniment on-device.
- **Synced Lyrics** — Load timed lyrics from online sources, embedded tags, or sidecar `.lrc` files.
- **CD+G Sidecars** — Render same-name `.cdg` graphics during fullscreen playback when a track includes them.
- **Portable Library** — Self-contained library directory that works on NAS, USB drives, and across machines.
- **Cross-Platform** — Available on macOS, Windows, and Linux.
- **4-Stem Mixer** — Individual volume control for vocals, drums, bass, and other instruments. Collapsible accompaniment slider with per-stem breakdown.
- **Dual Separation Modes** — Choose between 2-stem (vocals + accompaniment) or 4-stem (vocals + drums + bass + other). Upgrade individual songs from 2-stem to 4-stem on demand.
- **Efficient Stem Storage** — Separated stems are cached compactly to keep library storage practical.
- **Resumable Separation** — Per-chunk checkpointing means separation resumes from where it left off if the app is closed mid-process.

## Quick Start

### Install from Release

Download the latest build for your platform from [GitHub Releases](https://github.com/thedavidweng/OpenKara/releases):

| Platform              | Format                  |
| --------------------- | ----------------------- |
| macOS (Apple Silicon) | `.dmg`                  |
| macOS (Intel)         | `.dmg`                  |
| Windows               | `.exe` (NSIS installer) |
| Linux                 | `.AppImage` / `.deb`    |

**macOS (Homebrew):**

```bash
brew install thedavidweng/tap/openkara
```

**macOS Gatekeeper note:** If macOS says the app is damaged or can't be opened, run:

```bash
xattr -rd com.apple.quarantine /Applications/OpenKara.app
```

On first launch, OpenKara will prompt you to create a Karaoke Library and start downloading the default AI model in the background.

### Build from Source

**Prerequisites:**

- [Node.js](https://nodejs.org/) 20+
- [pnpm](https://pnpm.io/) 10+
- [Rust](https://rustup.rs/) stable toolchain
- Platform dependencies for [Tauri 2](https://v2.tauri.app/start/prerequisites/)

```bash
git clone https://github.com/thedavidweng/OpenKara.git
cd OpenKara
pnpm install
./scripts/setup.sh      # downloads Demucs ONNX model for local dev
pnpm tauri dev
```

### App Icon

- Source icon: `src-tauri/icons/app-icon.png` (`1024x1024` master asset)
- Regenerate all platform icons with `pnpm icons:generate`
- Generated assets are written to `src-tauri/icons/` for Tauri desktop and future mobile targets

## AI Models

OpenKara uses custom ONNX builds of [Demucs](https://github.com/adefossez/demucs) models for stem separation. Models are maintained in a separate repository:

**[openkara-models](https://github.com/thedavidweng/openkara-models)** — Reproducible ONNX model conversion pipeline

| Model         | Description                                | Input                           | Output                              | Format          |
| ------------- | ------------------------------------------ | ------------------------------- | ----------------------------------- | --------------- |
| `htdemucs`    | Standard — Hybrid Transformer Demucs       | Stereo audio at 44.1 kHz (7.8s) | 4 stems: drums, bass, other, vocals | ONNX (opset 17) |
| `htdemucs_ft` | High Quality — Fine-tuned 4-model ensemble | Stereo audio at 44.1 kHz (7.8s) | 4 stems: drums, bass, other, vocals | ONNX (opset 17) |

On first launch, OpenKara automatically downloads the standard model into the app data directory. The high quality model is optional and can be downloaded from Settings. See the [openkara-models README](https://github.com/thedavidweng/openkara-models#readme) for details on the conversion pipeline. For local development and deterministic tests, run `./scripts/setup.sh` to populate `src-tauri/models/`.

## Tech Stack

| Layer             | Technology                                                                                              | Purpose                         |
| ----------------- | ------------------------------------------------------------------------------------------------------- | ------------------------------- |
| Desktop framework | [Tauri 2](https://github.com/tauri-apps/tauri)                                                          | Rust backend + system WebView   |
| Frontend          | [React](https://github.com/facebook/react) 19 + [TypeScript](https://github.com/microsoft/TypeScript) 5 | UI components                   |
| Bundler           | [Vite](https://github.com/vitejs/vite) 7                                                                | Dev server and build            |
| Styling           | [Tailwind CSS](https://github.com/tailwindlabs/tailwindcss) 4                                           | Utility-first CSS               |
| State             | [Zustand](https://github.com/pmndrs/zustand)                                                            | Lightweight global state        |
| Audio decode      | [symphonia](https://github.com/pdeljanov/Symphonia)                                                     | Pure-Rust codec support         |
| Audio output      | [cpal](https://github.com/RustAudio/cpal)                                                               | Cross-platform audio playback   |
| AI inference      | [ONNX Runtime](https://github.com/microsoft/onnxruntime) via [ort](https://github.com/pykeio/ort)       | Demucs v4 stem separation       |
| Lyrics            | [LRCLIB](https://lrclib.net/)                                                                           | Open synced lyrics API          |
| Metadata          | [lofty](https://github.com/Serial-ATA/lofty-rs)                                                         | ID3v2, Vorbis, FLAC tag reading |
| Audio encode      | [vorbis_rs](https://github.com/ComunidadAylas/vorbis-rs)                                                | OGG/Vorbis stem compression     |
| Database          | [SQLite](https://github.com/sqlite/sqlite) via [rusqlite](https://github.com/rusqlite/rusqlite)         | Song, lyrics, and stems cache   |

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Tauri Frontend (React)                   │
│  ┌────────────────┐               ┌──────────────────────┐  │
│  │  File Import   │               │ Karaoke Player       │  │
│  │   & Library    │               │ / Mixer              │  │
│  │                │               │ (lyrics sync)        │  │
│  └────────────────┘               └──────────────────────┘  │
│  ┌────────────────┐               ┌──────────────────────┐  │
│  │   Playback     │               │ Progress & Volume    │  │
│  │   Controls     │               │ Controls             │  │
│  └────────────────┘               └──────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Tauri Rust Backend                       │
│  ┌────────────────┐               ┌──────────────────────┐  │
│  │   Audio Decode │               │  AI Stem Separation  │  │
│  │    & Playback  │               │ (Demucs v4 / ONNX)   │  │
│  └────────────────┘               └──────────────────────┘  │
│  ┌────────────────┐               ┌──────────────────────┐  │
│  │  Metadata      │               │   Lyrics Fetcher     │  │
│  │   Reader       │               │ (LRCLIB + embedded)  │  │
│  └────────────────┘               └──────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │             Portable Library                          │  │
│  │        (SQLite + media files + stems)                 │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Supported Formats

| Format       | Import | CD+G Graphics                  | Stem Separation |
| ------------ | ------ | ------------------------------ | --------------- |
| MP3          | ✅     | Same-name `.cdg` sidecar       | ✅              |
| FLAC         | ✅     | Same-name `.cdg` sidecar       | ✅              |
| WAV          | ✅     | Same-name `.cdg` sidecar       | ✅              |
| OGG / Vorbis | ✅     | Same-name `.cdg` sidecar       | ✅              |
| AAC / M4A    | ✅     | Same-name `.cdg` sidecar       | ✅              |
| MP3+G ZIP    | ✅     | Embedded audio + `.cdg` bundle | N/A             |

OpenKara imports same-name audio + `.cdg` pairs as managed CD+G tracks, and it can also import MP3+G ZIP archives directly. Standard audio tracks are resampled to 44.1 kHz stereo for the Demucs model. Managed CD+G tracks already contain accompaniment-only audio, so they skip stem separation.

## Portable Library

OpenKara stores all data in a self-contained library directory:

```
MyKaraokeLibrary/
├── .openkara-library       # marker file
├── openkara.db             # SQLite database
├── media/                  # imported standard audio copies
│   └── {hash}.mp3
├── media-g/                # managed CD+G assets
│   ├── {hash}.mp3          # paired audio for CD+G playback
│   ├── {hash}.cdg          # paired CD+G graphics sidecar
│   └── {hash}.zip          # MP3+G ZIP archive when imported as a bundle
└── stems/                  # separated tracks for standard audio imports
    └── {hash}/
        ├── vocals.ogg
        ├── accompaniment.ogg   # 2-stem mode
        ├── drums.ogg           # 4-stem mode
        ├── bass.ogg            # 4-stem mode
        └── other.ogg           # 4-stem mode
```

All paths in the database are relative — including CD+G sidecars and MP3+G ZIP assets — so the whole library can be moved to a NAS, USB drive, or network share and opened by any OpenKara instance on any OS. Per-machine configuration (library location) is stored separately in the app data directory.

## Roadmap

### ✅ v0.1 — MVP

- [x] Project scaffolding (Tauri 2 + React + TypeScript + Vite)
- [x] SQLite database with migration system
- [x] Audio import with metadata extraction (ID3v2, Vorbis, FLAC)
- [x] Library search and browsing
- [x] Audio decode and playback (symphonia + cpal)
- [x] Playback state machine (play / pause / seek / volume)
- [x] Demucs v4 ONNX stem separation with progress tracking
- [x] Stems caching (hash-based, no re-inference on replay)
- [x] Karaoke mode toggle (original / instrumental)
- [x] Synced lyrics fetch (LRCLIB → embedded → sidecar .lrc)
- [x] Lyrics display with rAF-based sync and click-to-seek
- [x] Per-song lyrics timing offset
- [x] First-launch AI model bootstrap with background download
- [x] Portable library system with relative paths
- [x] Full frontend UI (sidebar, player, lyrics panel, settings)
- [x] Queue panel with play next, drag reorder, and auto-advance
- [x] Keyboard shortcuts (space, arrows)
- [x] Drag-and-drop file import
- [x] CI/CD pipeline (macOS, Windows, Linux)
- [x] Release automation (tag → GitHub Release with binaries)

### 🚧 v0.3.0 — Current Development Version

The repository now tracks `0.3.0` development builds. Public release
automation, package metadata, and app-facing version information should use
`0.3.0` while the release scope is being defined.

- [x] CD+G sidecar playback for same-name audio + `.cdg` pairs
- [x] MP3+G ZIP import and playback support
- [x] Managed CD+G library storage and pairing disambiguation
- [x] Second-display fullscreen audience window
- [x] 4-stem volume mixer with collapsible UI
- [x] Dual separation modes (2-stem / 4-stem) with settings persistence
- [x] Efficient compressed stem storage
- [x] Resumable separation with per-chunk checkpointing
- [x] Multi-threaded ONNX inference optimization
- [x] Settings system (stem mode configuration)
- [x] UI polish and transitions
- [x] Error toasts and user-facing error messages
- [x] App icon and branding

### 📋 Future

- **Mic Input & Vocal Effects** — Microphone capture, reverb, echo, volume mix
- **Saved Playlists & Singer Rotation** — Named playlists, singer assignment, and stronger turn-based queue workflows
- **Pitch & Key Shift** — Real-time pitch shifting of the accompaniment track
- **Session Recording** — Record vocal performances, export as audio
- **CJK Transliteration** — Romaji / Pinyin display alongside original lyrics

## Development

### Prerequisites

- Node.js 20+
- pnpm 10+
- Rust stable via [rustup](https://rustup.rs/)
- [Tauri 2 prerequisites](https://v2.tauri.app/start/prerequisites/) for your platform

### Setup

```bash
pnpm install
./scripts/setup.sh          # download Demucs ONNX model to src-tauri/models/
pnpm tauri dev               # start dev server with hot reload
```

`scripts/setup.sh` places the model in `src-tauri/models/` for local development and deterministic testing only. End-user installs use the app data directory for runtime model downloads.

### Running Tests

```bash
cd src-tauri && cargo test   # backend tests (70+ tests)
pnpm lint                    # ESLint
pnpm format                  # Prettier check
```

### Building

```bash
pnpm tauri build             # production build with platform-specific bundle
```

### CI/CD

- Pushes to `main` trigger the CI workflow ([`.github/workflows/ci.yml`](./.github/workflows/ci.yml)) — lint, build, and test on macOS, Windows, and Linux.
- Pushing a version tag (e.g. `v0.3.0`) triggers the release workflow ([`.github/workflows/release.yml`](./.github/workflows/release.yml)) — builds and attaches binaries to a GitHub Release.

## Documentation

- [Docs Hub](./docs/README.md) — Canonical index for design docs, plans, specs, references, and site content
- [Architecture](./docs/design-docs/architecture.md) — System design, tech stack, data flow, and runtime details
- [Project Structure](./docs/design-docs/project-structure.md) — Current directory layout and module responsibilities
- [Development Phases](./docs/design-docs/development-phases.md) — Phase checklist with verification steps
- [Technical Roadmap](./docs/design-docs/roadmap.md) — Technology choices, API contracts, and risk mitigations
- [Milestones](./docs/design-docs/milestones.md) — Milestone task table with exit criteria

## Contributing

Contributions are welcome! Please open an issue before starting major changes so we can discuss the approach.

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes and ensure tests pass (`cargo test`)
4. Submit a pull request

## Acknowledgments

- [Demucs](https://github.com/adefossez/demucs) — AI stem separation model by Meta Research
- [openkara-models](https://github.com/thedavidweng/openkara-models) — ONNX model conversion pipeline for OpenKara
- [demucs.onnx](https://github.com/sevagh/demucs.onnx) — Reference for STFT/ISTFT to real-valued ONNX conversion
- [LRCLIB](https://lrclib.net) — Open synced lyrics API
- [monochrome](https://github.com/monochrome-music/monochrome) — Lyrics sync and LRCLIB integration reference

## License

[MIT](./LICENSE) — Copyright (c) 2025 David Weng
