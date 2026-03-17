# Architecture

## Overview

OpenKara is a cross-platform desktop karaoke application built with Tauri 2. It combines a Rust backend for audio processing and AI inference with a React frontend for the karaoke UI.

```
User's local music files
        │
        ▼
┌──────────────────────────────────────────────┐
│              Tauri Frontend (React)           │
│                                               │
│  ┌────────────┐  ┌─────────────────────────┐ │
│  │ File Import │  │     Karaoke Player UI   │ │
│  │ & Library   │  │  (lyrics sync/highlight)│ │
│  ├────────────┤  ├─────────────────────────┤ │
│  │  Playback   │  │   Progress & Volume     │ │
│  │  Controls   │  │   Controls              │ │
│  └────────────┘  └─────────────────────────┘ │
├──────────────────────────────────────────────┤
│              Tauri Rust Backend               │
│                                               │
│  ┌────────────┐  ┌─────────────────────────┐ │
│  │   Audio     │  │    AI Stem Separation   │ │
│  │   Decode &  │  │    (Demucs v4 via       │ │
│  │   Playback  │  │     ONNX Runtime)       │ │
│  ├────────────┤  ├─────────────────────────┤ │
│  │  Metadata   │  │    Lyrics Fetcher       │ │
│  │  Reader     │  │    (LRCLIB API +        │ │
│  │  (ID3/Flac) │  │     embedded lyrics)    │ │
│  ├────────────┴──┴─────────────────────────┤ │
│  │         Cache Layer (SQLite + fs)        │ │
│  │   separated stems / lyrics / metadata    │ │
│  └──────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

## Tech Stack

| Layer             | Technology                                           |
| ----------------- | ---------------------------------------------------- |
| Desktop framework | Tauri 2 (Rust + WebView)                             |
| Frontend          | React + TypeScript + Vite                            |
| Audio decode      | symphonia (Rust)                                     |
| Audio playback    | cpal (Rust)                                          |
| AI inference      | ONNX Runtime (ort crate)                             |
| AI model          | Demucs v4 (HTDemucs), ONNX export                    |
| Lyrics sources    | LRCLIB, embedded tags, sidecar `.lrc`, manual import |
| Metadata          | lofty (Rust, ID3/Vorbis/FLAC tags)                   |
| Cache / DB        | SQLite via rusqlite                                  |
| Build / Bundle    | Tauri CLI + Vite                                     |
| Distribution      | GitHub Releases, Homebrew                            |

## Data Flow

### Import & Separation

```
1. User drags audio file into the app
2. Rust backend reads file metadata (title, artist, album art)
3. Rust backend checks cache for existing separated stems
4. If cache miss:
   a. Decode audio to PCM (symphonia)
   b. Run Demucs v4 ONNX model → vocals + accompaniment
   c. Write separated stems to cache directory
5. Return metadata + stem paths to frontend
```

### Lyrics Fetch

```
1. Use song title + artist from metadata
2. Query LRCLIB for synced LRC lyrics
3. If not found, check for embedded lyrics in audio file tags
4. If not found, check for a same-basename `.lrc` sidecar file
5. If still not found, return no online result and let the user import or enter lyrics manually
6. Cache resolved lyrics in SQLite (source: "lrclib" | "embedded" | "sidecar" | "manual")
```

### Playback

```
1. Frontend sends play command with song ID
2. Rust backend streams the accompaniment stem to audio output
3. Frontend receives current playback position via event stream
4. Frontend highlights the current lyric line based on LRC timestamps
5. User clicks a lyric line → seek to that timestamp
```

## Lyrics System

> Reference implementation: [monochrome](https://github.com/monochrome-music/monochrome) (`js/lyrics.js`)
> monochrome is a production music player with a well-tested synced lyrics system. The design below draws from its proven approach.

### LRC Format

LRC is the standard synced lyrics format. Each line carries a timestamp:

```
[00:12.34] First line of lyrics
[00:17.89] Second line of lyrics
```

- Timestamp precision: centiseconds (0.01s), format `[MM:SS.CC]`
- Parse regex: `/\[(\d+):(\d+)\.(\d+)\]\s*(.+)/`
- Parsed into array of `{ time: number, text: string, words?: WordToken[] }` objects
- Line-level sync (entire line highlights at once)
- Enhanced LRC: word-level timing via `WordToken { time_ms, text }` for per-word highlighting

### LRCLIB API

Primary lyrics source. Free, open, no API key required.

```
GET https://lrclib.net/api/get?track_name={title}&artist_name={artist}&album_name={album}&duration={seconds}
```

- Returns JSON with `syncedLyrics` field (LRC string) and `plainLyrics` fallback
- `album_name` and `duration` are optional but improve match accuracy
- Matching is done server-side by metadata, not audio fingerprint

### Lyrics Fetch Priority

| Priority | Source                  | Notes                                                               |
| -------- | ----------------------- | ------------------------------------------------------------------- |
| 1        | LRCLIB API              | Best coverage for synced lyrics                                     |
| 2        | Embedded lyrics in tags | ID3v2 SYLT/USLT or Vorbis lyrics tags extracted from the audio file |
| 3        | Sidecar `.lrc` file     | Same directory, same basename as the audio file                     |
| 4        | LRC file import         | User imports `.lrc` files and the app matches them to songs         |
| 5        | Manual input            | User-pasted or manually edited plain text or LRC                    |

### Playback Sync Mechanism

> Learned from monochrome's `setupSync()` — a high-precision approach using `requestAnimationFrame` + `performance.now()`.

The naive approach (relying solely on `timeupdate` events) has two problems:

- `timeupdate` fires only ~4 times per second (every ~250ms)
- Timing jitter makes lyric transitions feel choppy

The proven solution:

```
┌─ Audio Events ──────────────────────────────────┐
│ play / seeked / timeupdate                      │
│   → Record baseTimeMs = currentTime * 1000      │
│   → Record lastTimestamp = performance.now()     │
└─────────────────────────────────────────────────┘
        │
        ▼
┌─ requestAnimationFrame Loop (60 FPS) ──────────┐
│ Each frame:                                     │
│   elapsed = performance.now() - lastTimestamp   │
│   currentMs = baseTimeMs + elapsed              │
│   lyricsComponent.currentTime = currentMs       │
│                                                 │
│ On pause → cancelAnimationFrame                 │
│ On play  → restart loop                         │
└─────────────────────────────────────────────────┘
```

Key points:

- `performance.now()` provides sub-millisecond precision for interpolation between audio events
- The loop runs only while audio is playing (paused → cancel)
- `timeupdate` and `seeked` events re-anchor `baseTimeMs` to prevent drift

### Per-Song Timing Offset

Users can adjust lyrics timing per song (e.g., lyrics arrive 0.5s early):

- Stored in cache: `lyrics_offset_{song_hash}` → offset in milliseconds
- Applied during sync: `currentMs = baseTimeMs + elapsed - timingOffset`
- Positive offset = lyrics delayed, negative = lyrics advanced
- UI: +/- buttons with 0.5s increments and a reset button

### CJK Language Support (Future)

monochrome supports Japanese → Romaji conversion via kuroshiro/kuromoji. For OpenKara, CJK considerations include:

- Detect CJK characters in lyrics text (Unicode ranges: `\u3040-\u309F`, `\u30A0-\u30FF`, `\u4E00-\u9FFF`, `\uAC00-\uD7AF`)
- Optional Romaji/Pinyin transliteration display
- Font fallback chain for CJK glyphs

This is a nice-to-have, not MVP scope.

## AI Model Details

### Demucs v4 (HTDemucs)

- **Purpose**: Separate audio into vocals and accompaniment (drums + bass + other)
- **License**: MIT
- **Input**: Raw PCM audio (44100 Hz, stereo)
- **Output**: 4 stems (vocals, drums, bass, other). We mix drums + bass + other into a single accompaniment track.
- **Model size**: ~80 MB (ONNX)
- **Inference time**: ~30-60s per 4-min song on Apple Silicon, ~2-3 min on older CPUs
- **Runtime**: ONNX Runtime with CPU execution provider (GPU optional via CoreML/DirectML)

### Why Demucs

- Best open-source separation quality (SDR benchmarks)
- MIT licensed
- Well-documented ONNX export path
- Active maintenance by Meta Research

### Alternatives Considered

| Model      | Pros                     | Cons                          |
| ---------- | ------------------------ | ----------------------------- |
| Open-Unmix | Lighter weight           | Lower separation quality      |
| Spleeter   | Fast, well-known         | Outdated, lower quality       |
| BSRNN      | State-of-the-art quality | Larger model, slower, complex |

## Caching Strategy

All expensive computations are cached to avoid redundant processing:

- **Separated stems**: Stored as OGG/Vorbis files in `~/.openkara/cache/stems/{hash}/`
- **Lyrics**: Stored in SQLite with song hash as key
- **Metadata**: Stored in SQLite for library browsing
- **Timing offsets**: Stored in SQLite per song hash

The cache key is a SHA-256 hash of the audio file content, ensuring deduplication even if files are renamed or moved.

## Platform Considerations

| Platform | Audio Backend   | AI Acceleration     |
| -------- | --------------- | ------------------- |
| macOS    | CoreAudio       | CoreML (optional)   |
| Windows  | WASAPI          | DirectML (optional) |
| Linux    | PulseAudio/ALSA | CPU only (default)  |

ONNX Runtime CPU execution provider works on all platforms out of the box. Hardware acceleration is a future optimization.
