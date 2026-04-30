# Technical Implementation Roadmap

> This document maps each subsystem to concrete technologies, crate/package choices, API contracts, and known technical risks. Historical phase checklists live in [archive/plans/development-phases.md](../archive/plans/development-phases.md).

---

## 1. Desktop Shell — Tauri 2

### Why Tauri

| Concern       | Electron              | Tauri 2                    |
| ------------- | --------------------- | -------------------------- |
| Bundle size   | ~150 MB+              | ~5–10 MB                   |
| Memory        | Chromium per window   | System WebView             |
| Native perf   | Node.js bridge        | Rust native                |
| Audio/AI perf | Child process or WASM | Native Rust, zero overhead |

### Key Config

```
src-tauri/tauri.conf.json
├── identifier: "com.openkara.desktop"
├── windows[0].title: "OpenKara"
├── security.csp: "default-src 'self'; connect-src 'self' https://lrclib.net"
└── bundle.targets: ["dmg", "nsis", "appimage"]
```

### IPC Contract

Frontend ↔ Backend communication via Tauri commands. All commands are async and return `Result<T, String>`.

```
Commands (src-tauri/src/commands/):
┌───────────────────────────┬──────────────────────────────────┐
│ Command                   │ Signature                        │
├───────────────────────────┼──────────────────────────────────┤
│ import_songs              │ (paths: Vec<String>) → Vec<Song> │
│ get_library               │ () → Vec<Song>                   │
│ search_library            │ (query: String) → Vec<Song>      │
│ play                      │ (song_id: String) → ()           │
│ pause                     │ () → ()                          │
│ seek                      │ (ms: u64) → ()                   │
│ set_volume                │ (level: f32) → ()                │
│ separate                  │ (song_id: String) → ()           │
│ get_separation_status     │ (song_id: String) → Status       │
│ fetch_lyrics              │ (song_id: String) → Lyrics       │
│ set_lyrics_offset         │ (song_id: String, ms: i64) → ()  │
│ set_playback_mode         │ (mode: "original"|"karaoke") → ()│
│ set_stem_volume           │ (stem: StemName, level: f32) → Snapshot │
│ load_stems                │ () → Snapshot                           │
│ get_settings              │ () → AppSettings                        │
│ set_stem_mode             │ (mode: String) → AppSettings            │
│ upgrade_to_four_stem      │ (song_id: String) → Status              │
│ save_manual_lyrics        │ (song_id: String, lrc: String) → ()     │
│ import_lyrics_files       │ (paths: Vec<String>) → ()               │
│ get_song_properties       │ (song_id: String) → SongProperties      │
│ get_all_separation_statuses│ () → Vec<SeparationStatus>             │
└───────────────────────────┴──────────────────────────────────┘

Events (Backend → Frontend):
┌───────────────────────────┬──────────────────────────────────┐
│ Event                     │ Payload                          │
├───────────────────────────┼──────────────────────────────────┤
│ playback-position         │ { ms: u64 }                      │
│ playback-ended            │ { song_id: String }              │
│ separation-progress       │ { song_id: String, percent: u8 } │
│ separation-complete       │ { song_id: String }              │
│ separation-error          │ { song_id: String, error: String}│
│ model-bootstrap-progress  │ { state, model_path, bytes... }  │
│ model-bootstrap-ready     │ { state, model_path }            │
│ model-bootstrap-error     │ { state, model_path, error }     │
└───────────────────────────┴──────────────────────────────────┘
```

---

## 2. Audio Pipeline — Rust

### Decode: `symphonia`

```
Input: MP3, FLAC, WAV, OGG, AAC, M4A
       │
       ▼
symphonia::core::formats::FormatReader
       │
       ▼
symphonia::core::codecs::Decoder
       │
       ▼
Output: PCM f32, 44100 Hz, stereo (resample if needed via rubato crate)
```

- **Why symphonia**: Pure Rust, no FFmpeg dependency, broad codec support.
- **Fallback**: If a format isn't supported, return an error to the user with the file extension.

### Playback: `cpal`

```
PCM samples
    │
    ▼
cpal::Stream (platform-specific backend)
    ├── macOS: CoreAudio
    ├── Windows: WASAPI
    └── Linux: PulseAudio / ALSA
```

- Ring buffer between decode thread and playback thread.
- Seek = flush ring buffer + re-decode from target position.
- Volume = multiply samples by `0.0..1.0` before writing to output.

### Rust Crates

| Crate       | Version | Purpose                             |
| ----------- | ------- | ----------------------------------- |
| `tauri`     | 2.x     | Desktop framework                   |
| `symphonia` | 0.5+    | Audio decode                        |
| `cpal`      | 0.15+   | Audio output                        |
| `ort`       | 2.x     | ONNX Runtime binding                |
| `lofty`     | 0.21+   | Audio tag reading                   |
| `rusqlite`  | 0.31+   | SQLite                              |
| `reqwest`   | 0.12+   | HTTP client (lyrics API)            |
| `tokio`     | 1.x     | Async runtime                       |
| `sha2`      | 0.10+   | File hashing for cache keys         |
| `serde`     | 1.x     | Serialization                       |
| `rubato`    | 0.15+   | Sample rate conversion (if needed)  |
| `vorbis_rs` | 0.5     | OGG/Vorbis encoding for stem output |

---

## 3. AI Stem Separation

### Model Pipeline

```
Audio file (any format)
    │
    ▼ symphonia decode
PCM f32, 44100 Hz, stereo
    │
    ▼ chunk into segments (Demucs expects fixed-length windows)
N segments × 2 channels × T samples
    │
    ▼ ort::Session::run()
4 stems per segment: vocals, drums, bass, other
    │
    ▼ overlap-add to reconstruct full-length stems
4 full-length stem arrays
    │
    ▼ mix drums + bass + other
2-stem mode: vocals.ogg, accompaniment.ogg
4-stem mode: vocals.ogg, drums.ogg, bass.ogg, other.ogg
    │
    ▼ write to cache (OGG/Vorbis compressed)
{library}/stems/{sha256_hash}/
```

### Technical Risks & Mitigations

| Risk                              | Impact                     | Mitigation                                                                      |
| --------------------------------- | -------------------------- | ------------------------------------------------------------------------------- |
| ONNX model too large for download | Users abandon setup        | Host model on GitHub Releases (LFS or release asset). Provide SHA-256 checksum. |
| Inference too slow on old CPUs    | Bad UX                     | Show progress bar. Process in background. Cache aggressively.                   |
| ONNX Runtime version mismatch     | Build fails                | Pin `ort` crate version. Test in CI on all platforms.                           |
| Model produces artifacts          | Audio glitches             | Use overlap-add with crossfade between chunks. Tune chunk/overlap size.         |
| Memory spike during inference     | App crash on 8 GB machines | Process in chunks, not entire song at once. Monitor peak RSS.                   |

### Model Distribution Strategy

Current `htdemucs.onnx` is large enough that it should be treated as a
runtime asset, not a repository artifact.

Rules:

1. Keep `.onnx` files out of git. The development repo should only retain
   `src-tauri/models/.gitkeep` and documentation.
2. Local developers can prewarm `src-tauri/models/` via `scripts/setup.sh` for
   deterministic tests.
3. End users should prefer a runtime install under the app data directory.
4. The product UX should be:
   - check on startup whether a verified model already exists
   - if missing, show a clear prompt with size, storage location, and
     `Download now / Later`
   - if the user accepts, download in the background with progress and retry
   - if the user skips, keep library/playback usable but block
     separation/karaoke until the model is ready
   - if the user enters karaoke before the model is installed, prompt again

### Performance Targets

| Metric                       | Target  | Acceptable |
| ---------------------------- | ------- | ---------- |
| 4-min song, Apple Silicon M1 | < 45s   | < 90s      |
| 4-min song, Intel i5 (2020)  | < 120s  | < 180s     |
| Peak memory during inference | < 2 GB  | < 3 GB     |
| Cache hit playback start     | < 500ms | < 1s       |

---

## 4. Lyrics System

### Fetch Pipeline

```
Song metadata (title, artist, album, duration)
    │
    ├──→ [1] LRCLIB API (GET https://lrclib.net/api/get?...)
    │        └─ Returns: { syncedLyrics: "[00:12.34] ...", plainLyrics: "..." }
    │
    ├──→ [2] Embedded tags (ID3v2 SYLT/USLT, Vorbis LYRICS)
    │        └─ Read via lofty crate
    │
    └──→ [3] Sidecar .lrc file (same dir, same name as audio)
             └─ Read from filesystem

    Result: LRC string or "no lyrics found"
    │
    ├──→ [4] Manual input (user-typed plain text or LRC, auto-detected)
    │
    ▼ parse
    Vec<LyricLine { time_ms: u64, text: String, words: Option<Vec<WordToken>> }>
    │   Enhanced LRC: per-word timing via WordToken for word-level highlighting
    │
    ▼ cache in SQLite
    lyrics table: (song_hash TEXT PK, lrc TEXT, source TEXT, fetched_at INTEGER)
    source: "lrclib" | "embedded" | "sidecar" | "file_import" | "manual"
```

### Frontend Sync Architecture

```
             Rust backend (cpal)
                    │
        emit "playback-position" { ms }
              (on timeupdate / seek)
                    │
                    ▼
        ┌─ useLyricsSync hook ─────────────────────────┐
        │                                               │
        │  On position event:                           │
        │    baseTimeMs = event.ms                      │
        │    lastTimestamp = performance.now()           │
        │                                               │
        │  requestAnimationFrame loop:                  │
        │    elapsed = performance.now() - lastTimestamp │
        │    currentMs = baseTimeMs + elapsed - offset  │
        │    activeLineIndex = binary search in lyrics  │
        │                                               │
        │  On pause: cancel rAF                         │
        │  On seek:  re-anchor baseTimeMs               │
        └───────────────────────────────────────────────┘
                    │
                    ▼
        Lyrics component re-renders active line only
        (CSS transition for scroll, highlight color change)
```

---

## 5. Frontend Architecture

### Stack

| Concern   | Choice         | Rationale                           |
| --------- | -------------- | ----------------------------------- |
| Framework | React 19       | Ecosystem, hiring pool              |
| Language  | TypeScript 5   | Type safety                         |
| Bundler   | Vite 6         | Fast HMR, Tauri integration         |
| Styling   | Tailwind CSS 4 | Utility-first, no CSS-in-JS runtime |
| State     | Zustand        | Minimal API, no boilerplate         |
| Icons     | Lucide React   | Consistent, tree-shakeable          |

### Component Tree

```
App
├── Library
│   ├── SearchBar
│   ├── SongGrid
│   │   └── SongCard (cover art, title, artist, separation badge)
│   ├── ImportDropzone
│   └── SongPropertiesDialog (format, sample rate, channels, bitrate, size)
├── NowPlaying (full-screen karaoke view)
│   ├── Lyrics
│   │   ├── LyricLine (highlighted / upcoming / past, word-level highlight)
│   │   └── OffsetControls (+/- 0.5s buttons)
│   ├── LyricsEditDialog (manual lyrics input, LRC vs plain text auto-detection)
│   └── Player
│       ├── PlayPauseButton
│       ├── SkipForwardButton / SkipBackButton
│       ├── SeekBar
│       ├── VolumeSlider (click icon to mute/unmute)
│       ├── ModeToggle (original / karaoke)
│       ├── QueueButton
│       └── QueuePanel (drag-to-reorder, auto-advance)
├── Settings
│   └── StemModeToggle (2-stem / 4-stem)
└── StatusBar
    └── SeparationProgress
```

### State Stores (Zustand)

```typescript
// playerStore: playback state
{ songId, isPlaying, positionMs, volume, mode: "original" | "karaoke" }

// libraryStore: song collection
{ songs: Song[], searchQuery, isImporting }

// lyricsStore: current lyrics
{ lines: LyricLine[], activeIndex, offset }

// queueStore: playback queue
{ queue: Song[], addToQueue, playNext, dequeue, reorder, autoAdvance }
```

> **Future candidate**: Configurable crossfade duration with dual-track rendering in the audio output backend.

---

## 6. Data Layer — SQLite

### Schema

```sql
-- 001_init.sql

CREATE TABLE songs (
    hash        TEXT PRIMARY KEY,    -- SHA-256 of file content
    file_path   TEXT NOT NULL,
    title       TEXT,
    artist      TEXT,
    album       TEXT,
    duration_ms INTEGER,
    cover_art   BLOB,               -- thumbnail, max 500x500
    imported_at INTEGER NOT NULL     -- unix timestamp
);

CREATE TABLE lyrics (
    song_hash   TEXT PRIMARY KEY REFERENCES songs(hash),
    lrc         TEXT,                -- raw LRC string
    source      TEXT,                -- "lrclib" | "embedded" | "sidecar"
    offset_ms   INTEGER DEFAULT 0,  -- user timing adjustment
    fetched_at  INTEGER NOT NULL
);

CREATE TABLE stems (
    song_hash       TEXT PRIMARY KEY REFERENCES songs(hash),
    vocals_path     TEXT NOT NULL,
    accomp_path     TEXT NOT NULL,
    separated_at    INTEGER NOT NULL,
    drums_path      TEXT,
    bass_path       TEXT,
    other_path      TEXT
);
```

---

## 7. Build & Distribution

### Build Pipeline

```
pnpm install          → install frontend deps
pnpm tauri build      → Vite build + Cargo build + bundle
                        │
                        ├── macOS: .dmg (arm64, x64)
                        ├── Windows: .msi / .exe (NSIS)
                        └── Linux: .AppImage, .deb
```

### CI/CD (GitHub Actions)

```yaml
# Trigger: push to main, pull request, tag push
Jobs:
  lint: pnpm lint + cargo clippy
  test: cargo test + vitest
  build: matrix [macos-arm64, macos-x64, windows, ubuntu]
  release: on tag v* → create GitHub Release with artifacts
```

### Model Distribution

The ONNX model (~289 MB) is not in the repo. Options:

| Strategy             | Pros                  | Cons                        |
| -------------------- | --------------------- | --------------------------- |
| GitHub Release asset | Simple, versioned     | 2 GB per-release limit      |
| First-run download   | Small initial install | Needs internet on first use |
| CDN (Cloudflare R2)  | Fast global download  | Requires infra              |

**MVP choice**: First-run download from GitHub Release asset. Display download progress in the app.

---

## Technology Decision Log

| Decision          | Chosen                                                         | Alternatives Considered        | Reason                                            |
| ----------------- | -------------------------------------------------------------- | ------------------------------ | ------------------------------------------------- |
| Desktop framework | [Tauri 2](https://github.com/tauri-apps/tauri)                 | Electron, Flutter              | Bundle size, Rust native perf for audio/AI        |
| Audio decode      | [symphonia](https://github.com/pdeljanov/Symphonia)            | FFmpeg binding                 | Pure Rust, no C dependency hell                   |
| Audio output      | [cpal](https://github.com/RustAudio/cpal)                      | rodio                          | Lower-level control needed for seek/buffer        |
| AI runtime        | [ONNX Runtime](https://github.com/microsoft/onnxruntime) (ort) | PyTorch C++, TFLite            | Broadest hardware support, Demucs has ONNX export |
| Lyrics API        | LRCLIB                                                         | Musixmatch, Genius             | Free, no API key, good synced lyrics coverage     |
| State management  | [Zustand](https://github.com/pmndrs/zustand)                   | Redux, Jotai                   | Minimal boilerplate for small app                 |
| Styling           | [Tailwind CSS](https://github.com/tailwindlabs/tailwindcss)    | styled-components, CSS Modules | No runtime, great DX                              |
| Database          | [SQLite](https://github.com/sqlite/sqlite) (rusqlite)          | sled, redb                     | Mature, SQL queries, Tauri plugin ecosystem       |
