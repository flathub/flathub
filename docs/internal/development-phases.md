# Development Phases

> Each phase is self-contained. An engineer joining at any phase can read the **Entry Checklist** to verify prerequisites, then follow the tasks in order.

---

## Phase 0 — Project Scaffolding

**Goal**: A buildable, runnable empty shell.

### Entry Checklist

- [ ] Node.js >= 20, pnpm >= 9 installed
- [ ] Rust toolchain (rustup) installed, stable channel
- [ ] Tauri CLI v2 installed (`cargo install tauri-cli --version "^2"`)

### Tasks

| #   | Task                                                                          | Output                                            | Verify                                                 |
| --- | ----------------------------------------------------------------------------- | ------------------------------------------------- | ------------------------------------------------------ |
| 0.1 | `pnpm create tauri-app` — init Tauri 2 + React + TypeScript + Vite            | `package.json`, `src-tauri/Cargo.toml` generated  | `pnpm install && pnpm tauri dev` opens an empty window |
| 0.2 | Configure `vite.config.ts` — path aliases (`@/` → `src/`)                     | Updated config                                    | `import {} from '@/types'` resolves                    |
| 0.3 | Add ESLint + Prettier configs                                                 | `.eslintrc.cjs`, `.prettierrc`                    | `pnpm lint` passes                                     |
| 0.4 | Add Tailwind CSS 4                                                            | `src/styles/globals.css` with Tailwind directives | Utility classes render in dev                          |
| 0.5 | Create SQLite migration infra                                                 | `src-tauri/migrations/001_init.sql`               | App starts without DB errors                           |
| 0.6 | Write `scripts/setup.sh` — downloads Demucs ONNX model to `src-tauri/models/` | Script file                                       | Run script → model file present                        |
| 0.7 | CI pipeline — GitHub Actions: lint, build, test                               | `.github/workflows/ci.yml`                        | Push → green check                                     |

---

## Phase 1 — Audio Import & Library

**Goal**: User can add local audio files, see them in a library, and read metadata.

### Entry Checklist

- [ ] Phase 0 complete — `pnpm tauri dev` works
- [ ] SQLite migration creates `songs` table

### Tasks

| #   | Task                                                                      | Output                                      | Verify                                             |
| --- | ------------------------------------------------------------------------- | ------------------------------------------- | -------------------------------------------------- |
| 1.1 | Rust: `metadata` module — read ID3v2, Vorbis, FLAC tags via `lofty` crate | `src-tauri/src/metadata/mod.rs`             | Unit test: parse test MP3 → title + artist correct |
| 1.2 | Rust: `cache` module — insert/query songs in SQLite                       | `src-tauri/src/cache/mod.rs`                | Unit test: insert then select → row exists         |
| 1.3 | Rust: Tauri command `import_songs(paths: Vec<String>)`                    | `src-tauri/src/commands/import.rs`          | Call from devtools → songs appear in DB            |
| 1.4 | Frontend: `Library` component — grid/list view of songs                   | `src/components/Library/`                   | UI shows imported songs with cover art             |
| 1.5 | Frontend: drag-and-drop / file-picker import flow                         | `src/components/Library/ImportDropzone.tsx` | Drag MP3 onto window → song appears in library     |
| 1.6 | Frontend: search + filter in library                                      | Search bar in Library                       | Type artist name → results filter live             |

---

## Phase 2 — Audio Playback

**Goal**: User can play, pause, seek, and control volume for any song in the library (original audio, no separation yet).

### Entry Checklist

- [ ] Phase 1 complete — songs appear in library with metadata
- [ ] `symphonia` and `cpal` crates added to `Cargo.toml`

### Tasks

| #   | Task                                                               | Output                              | Verify                                                   |
| --- | ------------------------------------------------------------------ | ----------------------------------- | -------------------------------------------------------- |
| 2.1 | Rust: `audio` module — decode audio to PCM via `symphonia`         | `src-tauri/src/audio/decode.rs`     | Unit test: decode 10s of test WAV → correct sample count |
| 2.2 | Rust: `audio` module — playback via `cpal` output stream           | `src-tauri/src/audio/playback.rs`   | Tauri command `play(song_id)` → audio plays from speaker |
| 2.3 | Rust: playback state machine — play / pause / stop / seek          | State enum + commands               | Rapid play-pause-seek sequence → no crashes              |
| 2.4 | Rust: emit playback position events (Tauri event system, ~60 Hz)   | `emit("playback-position", { ms })` | Frontend console logs position updates                   |
| 2.5 | Frontend: `Player` component — play/pause button, seek bar, volume | `src/components/Player/`            | Click play → audio plays, drag seek bar → audio jumps    |
| 2.6 | Frontend: global playback store (Zustand)                          | `src/stores/playerStore.ts`         | Components react to play state changes                   |

---

## Phase 3 — AI Stem Separation

**Goal**: User clicks a song, the app separates vocals from accompaniment, caches the result, and plays the instrumental track.

### Entry Checklist

- [ ] Phase 2 complete — original playback works
- [ ] For local backend work, a verified ONNX model is available either:
  - in `src-tauri/models/` via `scripts/setup.sh`, or
  - in the app data directory via runtime bootstrap
- [ ] `ort` crate added to `Cargo.toml`

### Tasks

| #   | Task                                                                       | Output                                     | Verify                                                     |
| --- | -------------------------------------------------------------------------- | ------------------------------------------ | ---------------------------------------------------------- |
| 3.1 | Rust: `separator` module — load Demucs ONNX model                          | `src-tauri/src/separator/model.rs`         | Unit test: model loads, session creates without error      |
| 3.2 | Rust: `separator` — preprocess PCM → model input tensor                    | `src-tauri/src/separator/preprocess.rs`    | Correct tensor shape for Demucs input                      |
| 3.3 | Rust: `separator` — run inference, postprocess → stems                     | `src-tauri/src/separator/inference.rs`     | Integration test: 10s audio → 4 stem WAVs written          |
| 3.4 | Rust: mix drums + bass + other → single accompaniment WAV                  | `src-tauri/src/separator/mix.rs`           | Output file plays, sounds like instrumental                |
| 3.5 | Rust: cache stems — hash-based directory `~/.openkara/cache/stems/{hash}/` | `src-tauri/src/cache/stems.rs`             | Second run → cache hit, no re-inference                    |
| 3.6 | Rust: separation progress events (% complete)                              | `emit("separation-progress", { percent })` | Frontend shows progress                                    |
| 3.7 | Frontend: separation status indicator + play toggle (original / karaoke)   | Player UI update                           | User sees progress bar, then can toggle vocal/instrumental |
| 3.8 | Rust: background separation with async channel                             | `tokio::spawn` + channel                   | UI remains responsive during 60s inference                 |

---

## Phase 4 — Synced Lyrics

**Goal**: Synced lyrics display, highlighting the current line in time with playback.

### Entry Checklist

- [ ] Phase 2 complete — playback position events emitting at ~60 Hz
- [ ] Network access available for LRCLIB API

### Tasks

| #   | Task                                                                      | Output                           | Verify                                                       |
| --- | ------------------------------------------------------------------------- | -------------------------------- | ------------------------------------------------------------ |
| 4.1 | Rust: `lyrics` module — LRCLIB API client                                 | `src-tauri/src/lyrics/lrclib.rs` | Unit test: fetch lyrics for known song → LRC string returned |
| 4.2 | Rust: LRC parser — `[MM:SS.CC] text` → `Vec<LyricLine { time_ms, text }>` | `src-tauri/src/lyrics/parser.rs` | Unit test: parse sample LRC → correct timestamps             |
| 4.3 | Rust: lyrics fetch priority chain (LRCLIB → embedded → sidecar .lrc)      | `src-tauri/src/lyrics/fetch.rs`  | Falls through sources correctly                              |
| 4.4 | Rust: cache lyrics in SQLite                                              | `lyrics` table + cache logic     | Second fetch → DB hit, no HTTP request                       |
| 4.5 | Frontend: `Lyrics` component — scrolling lyrics panel                     | `src/components/Lyrics/`         | Lyrics visible, scrolls as time passes                       |
| 4.6 | Frontend: `requestAnimationFrame` + `performance.now()` sync loop         | `src/hooks/useLyricsSync.ts`     | Current line highlights at correct moment, smooth transition |
| 4.7 | Frontend: click lyric line → seek to that timestamp                       | Click handler in Lyrics          | Click line → playback jumps to line start                    |
| 4.8 | Frontend: per-song timing offset (± buttons, 0.5s increments)             | Offset UI + persistence          | Offset persists after app restart                            |

---

## Phase 5 — Integration & Polish

**Goal**: All four systems work together seamlessly. The full karaoke experience end-to-end.

### Entry Checklist

- [ ] Phases 1–4 individually complete and tested
- [ ] At least 5 test songs with known lyrics available

### Tasks

| #    | Task                                                              | Output                         | Verify                                           |
| ---- | ----------------------------------------------------------------- | ------------------------------ | ------------------------------------------------ |
| 5.1  | End-to-end flow: import → separate → fetch lyrics → play karaoke  | Full flow works                | Manual walkthrough: 3 different songs succeed    |
| 5.2  | Error handling: missing lyrics, separation failure, corrupt files | Error toasts + fallback states | Bad MP3 → graceful error, not crash              |
| 5.3  | Performance profiling: playback latency, lyrics sync jitter       | Benchmark report               | Lyrics jitter < 50ms, seek latency < 200ms       |
| 5.4  | UI polish: transitions, loading states, responsive layout         | Visual improvements            | App looks good at 1280x800 and 1920x1080         |
| 5.5  | Keyboard shortcuts: space = play/pause, arrows = seek ±5s         | Keybindings                    | Shortcuts work without focus issues              |
| 5.6  | App icon, splash screen, window title                             | Assets + Tauri config          | Bundled app has proper branding                  |
| 5.7  | Update READMEs with install + usage instructions                  | Updated docs                   | New user can install and run from README alone   |
| 5.8  | Enhanced LRC parser with word-level timing (WordToken)            | Per-word highlighting          | Words highlight individually during playback     |
| 5.9  | LRC file import with auto-matching by filename/metadata           | `import_lyrics_files` command  | Import .lrc → auto-matches to songs              |
| 5.10 | Embedded lyrics extraction from audio files                       | Extraction during import       | Songs with embedded lyrics show them on play     |
| 5.11 | Manual lyrics input dialog (LRC/plain auto-detect)                | `LyricsEditDialog` component   | User types lyrics → saved and displayed          |
| 5.12 | Song properties dialog (format, sample rate, channels, etc.)      | `SongPropertiesDialog`         | Right-click → Properties shows file details      |
| 5.13 | Volume icon click-to-mute with previous volume memory             | Mute toggle in VolumeSlider    | Click speaker icon → mute, click again → restore |
| 5.14 | Playback queue system (queue store, panel, context menu)          | Queue subsystem                | Add to queue, reorder, auto-advance works        |
| 5.15 | Skip forward/back buttons + song transition animations            | Player transport controls      | Skip buttons navigate queue, fade-in on switch   |

---

## Phase 6 — Build, Test & Release

**Goal**: Distributable binaries for macOS, Windows, and Linux.

### Entry Checklist

- [ ] Phase 5 complete — full flow works
- [ ] GitHub Actions CI green

### Tasks

| #   | Task                                                                   | Output                     | Verify                                                                                              |
| --- | ---------------------------------------------------------------------- | -------------------------- | --------------------------------------------------------------------------------------------------- |
| 6.1 | Tauri build config — app ID, version, signing                          | `tauri.conf.json` complete | `pnpm tauri build` succeeds                                                                         |
| 6.2 | GitHub Actions: build matrix (macOS arm64, macOS x64, Windows, Linux)  | CI workflow                | All 4 targets produce artifacts                                                                     |
| 6.3 | GitHub Release automation — tag push triggers release with binaries    | Release workflow           | `git tag v0.1.0 && git push --tags` → Release created                                               |
| 6.4 | Model bootstrap UX — startup check, prompt, background download, retry | Setup logic + UX contract  | Fresh install → user can download now/later, karaoke blocked until ready                            |
| 6.5 | Smoke test on each platform                                            | Test report                | `./scripts/run-local-smoke.sh` on local corpus + app launch/playback smoke on macOS, Windows, Linux |
| 6.6 | Homebrew Cask distribution (macOS)                                     | Tap repo + cask file       | `brew install --cask openkara` works                                                                |

---

## Reading Guide

- **Each phase is independently testable.** The "Verify" column tells you exactly how to confirm a task is done.
- **Phases 3 and 4 are parallel.** Stem separation and lyrics fetch have no dependency on each other. Two engineers can work on them simultaneously after Phase 2.
- **Phase numbering = suggested order, not strict sequence.** Phase 0 and 1 are sequential. Phases 2–4 can overlap. Phase 5 requires all prior phases. Phase 6 is the final gate.
