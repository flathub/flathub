# Milestones

> Each milestone is a shippable increment. The **Demo** column describes what a stakeholder should see at the end of the milestone. The **Exit Criteria** column lists the hard requirements to move on.

---

## Overview

```
M0        M1          M2          M3          M4          M5         M6
 │         │           │           │           │           │          │
 ▼         ▼           ▼           ▼           ▼           ▼          ▼
Shell  ─→ Library ─→ Playback ─→ Separation ─→ Lyrics ─→ Polish ─→ Release
                                   ║                       ║
                                   ╚══════ parallel ══════╝
```

---

## M0 — Empty Shell

| Item              | Detail                                            |
| ----------------- | ------------------------------------------------- |
| **Goal**          | Project compiles and runs on all target platforms |
| **Phases**        | Phase 0                                           |
| **Demo**          | `pnpm tauri dev` → empty window with app title    |
| **Exit Criteria** | CI green (lint + build) on macOS, Windows, Linux  |

### Task Breakdown

| Task                             | Owner | Status | Notes                        |
| -------------------------------- | ----- | ------ | ---------------------------- |
| Init Tauri 2 + React + TS + Vite | Code  | ✅     | `pnpm create tauri-app`      |
| Tailwind CSS setup               | Code  | ✅     |                              |
| ESLint + Prettier config         | Code  | ✅     |                              |
| SQLite migration infra           | Code  | ✅     | `songs` / `stems` / `lyrics` |
| ONNX model download script       | Code  | ✅     | `scripts/setup.sh`           |
| GitHub Actions CI                | Code  | ✅     | lint, build, test matrix     |

---

## M1 — Music Library

| Item              | Detail                                                                                |
| ----------------- | ------------------------------------------------------------------------------------- |
| **Goal**          | User imports local music and browses a library                                        |
| **Phases**        | Phase 1                                                                               |
| **Demo**          | Drag 10 MP3s into app → grid with cover art, title, artist. Search filters live.      |
| **Exit Criteria** | Metadata reads correctly for MP3, FLAC, M4A. Songs persist in SQLite across restarts. |

### Task Breakdown

| Task                          | Owner | Status | Notes                    |
| ----------------------------- | ----- | ------ | ------------------------ |
| Metadata reader (lofty)       | Code  | ✅     | ID3v2, Vorbis, FLAC tags |
| SQLite songs CRUD             | Code  | ✅     | insert, query, delete    |
| `import_songs` Tauri command  | Code  | ✅     | Accept file paths        |
| Library UI — grid + list view | UI    | ✅     | Responsive layout        |
| Drag-and-drop import          | UI    | ✅     | + file picker fallback   |
| Library search + filter       | UI    | ✅     | By title, artist         |

---

## M2 — Audio Playback

| Item              | Detail                                                                                  |
| ----------------- | --------------------------------------------------------------------------------------- |
| **Goal**          | Full playback of original audio with transport controls                                 |
| **Phases**        | Phase 2                                                                                 |
| **Demo**          | Click a song → plays through speakers. Seek bar, volume, play/pause all work.           |
| **Exit Criteria** | Gapless decode for MP3/FLAC/WAV. Seek latency < 200ms. No audio glitches on rapid seek. |

### Task Breakdown

| Task                            | Owner | Status | Notes                        |
| ------------------------------- | ----- | ------ | ---------------------------- |
| Audio decode (symphonia)        | Code  | ✅     | MP3, FLAC, WAV, OGG, AAC     |
| Audio output (cpal)             | Code  | ✅     | Platform-specific backends   |
| Playback state machine          | Code  | ✅     | play / pause / stop / seek   |
| Position event emitter (~60 Hz) | Code  | ✅     | Tauri events                 |
| Player UI — controls            | UI    | ✅     | Play/pause, seek bar, volume |
| Zustand playerStore             | UI    | ✅     | Global playback state        |

---

## M3 — AI Stem Separation

| Item              | Detail                                                                                                                             |
| ----------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| **Goal**          | Any song can be separated into vocals + accompaniment (2-stem) or into individual instruments (4-stem). Cached for instant replay. |
| **Phases**        | Phase 3                                                                                                                            |
| **Demo**          | Click "Karaoke Mode" → progress bar → instrumental plays without vocals. Second play is instant.                                   |
| **Exit Criteria** | Separation completes for a 4-min song in < 90s on M1 Mac. Cache hit → < 500ms to play. Peak memory < 3 GB.                         |

### Task Breakdown

| Task                                | Owner | Status | Notes                                   |
| ----------------------------------- | ----- | ------ | --------------------------------------- |
| Load Demucs ONNX model              | Code  | ✅     | `ort::Session`                          |
| PCM → model input preprocessing     | Code  | ✅     | Chunking, tensor shape                  |
| Inference + overlap-add postprocess | Code  | ✅     | 4 stems → 2 or 4 outputs (configurable) |
| Stems cache (fs, hash-based)        | Code  | ✅     | `~/.openkara/cache/stems/`              |
| Background processing (tokio)       | Code  | ✅     | Non-blocking UI                         |
| Progress events                     | Code  | ✅     | Percent complete                        |
| Mode toggle UI (original / karaoke) | UI    | ✅     | Player component                        |

---

## M4 — Synced Lyrics

| Item              | Detail                                                                                                              |
| ----------------- | ------------------------------------------------------------------------------------------------------------------- |
| **Goal**          | Lyrics scroll in sync with playback, highlighted line-by-line                                                       |
| **Phases**        | Phase 4                                                                                                             |
| **Demo**          | Song plays → lyrics panel shows lines scrolling and highlighting in real time. Click a line → playback jumps there. |
| **Exit Criteria** | Lyrics sync jitter < 50ms. LRCLIB fetch success rate > 70% for English pop songs. Offset adjustment persists.       |

### Task Breakdown

| Task                         | Owner | Status | Notes                         |
| ---------------------------- | ----- | ------ | ----------------------------- |
| LRCLIB API client            | Code  | ✅     | HTTP GET with metadata params |
| LRC parser                   | Code  | ✅     | Regex parse → structured data |
| Fetch priority chain         | Code  | ✅     | LRCLIB → embedded → sidecar   |
| Lyrics SQLite cache          | Code  | ✅     | song_hash → lrc               |
| Lyrics UI component          | UI    | ✅     | Scrolling panel, highlight    |
| rAF + performance.now() sync | UI    | ✅     | useLyricsSync hook            |
| Click-to-seek on lyric line  | UI    | ✅     |                               |
| Timing offset controls       | UI    | ✅     | ± 0.5s, persisted             |

---

## M5 — Integration & Polish

| Item              | Detail                                                                                                                |
| ----------------- | --------------------------------------------------------------------------------------------------------------------- |
| **Goal**          | Complete, polished karaoke experience end-to-end                                                                      |
| **Phases**        | Phase 5                                                                                                               |
| **Demo**          | Import song → auto-separate → lyrics appear → sing along with instrumental. Feels like a real product.                |
| **Exit Criteria** | 5 different songs tested end-to-end without errors. Keyboard shortcuts work. Loading states for all async operations. |

### Task Breakdown

| Task                                        | Owner   | Status | Notes                                                          |
| ------------------------------------------- | ------- | ------ | -------------------------------------------------------------- |
| E2E flow testing (5+ songs)                 | Code    | ⏳     | 已有后端 smoke test；真实 5 首歌回归仍待完成                   |
| Error handling & user feedback              | Code/UI | ✅     | Structured errors, user-facing feedback                        |
| Performance profiling                       | Code    | ✅     | Latency, jitter, memory                                        |
| UI polish & transitions                     | UI      | ✅     | Fade-in animations, song transitions, smooth scroll            |
| Keyboard shortcuts                          | UI/Code | ✅     | Space, arrows, etc.                                            |
| App branding (icon, splash)                 | UI      | ✅     | Branded app icon assets added                                  |
| Documentation update                        | Code    | ⏳     | README 与交接文档已扩展，用户级安装指南仍可继续细化            |
| 4-stem mixer + dual modes + OGG compression | Code/UI | ✅     | Checkpoint resumability, stem mode settings, compressed output |

---

## M6 — v0.1.0 Release

| Item              | Detail                                                                                                          |
| ----------------- | --------------------------------------------------------------------------------------------------------------- |
| **Goal**          | Downloadable binaries on GitHub for macOS, Windows, Linux                                                       |
| **Phases**        | Phase 6                                                                                                         |
| **Demo**          | User downloads `.dmg` / `.exe` / `.AppImage`, installs, imports music, and sings karaoke.                       |
| **Exit Criteria** | Smoke test passes on all 3 platforms. GitHub Release with checksums published. README has install instructions. |

### Task Breakdown

| Task                     | Owner   | Status | Notes                                                                          |
| ------------------------ | ------- | ------ | ------------------------------------------------------------------------------ |
| Tauri build config       | Code    | ✅     | App ID, targets                                                                |
| CI build matrix          | Code    | ✅     | CI workflows run on macOS, Windows, Linux                                      |
| Release automation       | Code    | ✅     | Tag → GitHub Release with binaries                                             |
| First-run model download | Code/UI | ✅     | Bootstrap with background download, progress, and retry                        |
| Platform smoke tests     | Code    | ⏳     | `scripts/run-local-smoke.sh` 已支持本地语料回归；仍需补 Windows/Linux 启动记录 |
| Homebrew distribution    | Code    | ⏳     | 改为 Homebrew Cask 方向；仓库内已补 cask 模板与渲染脚本，tap repo 仍待接入     |

---

## M8 — Playlist & Queue

| Item              | Detail                                                                                                                                                          |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Goal**          | Users can queue songs, reorder the queue, and playback advances automatically between songs                                                                     |
| **Phases**        | Phase 5 (integrated alongside polish)                                                                                                                           |
| **Demo**          | Right-click song → "Play Next" or "Add to Queue". Queue panel shows upcoming songs with drag reorder. Song ends → next song auto-plays with fade-in transition. |
| **Exit Criteria** | Queue persists during session. Skip forward/back work. Auto-advance on song end.                                                                                |

### Task Breakdown

| Task                                   | Owner | Status | Notes                                                     |
| -------------------------------------- | ----- | ------ | --------------------------------------------------------- |
| Queue store (Zustand)                  | Code  | ✅     | `queue-store.ts` — addToQueue, playNext, dequeue, reorder |
| Queue panel UI                         | UI    | ✅     | `QueuePanel.tsx` — drag-to-reorder, remove items          |
| Queue button in player bar             | UI    | ✅     | `QueueButton.tsx` — toggle queue panel visibility         |
| Context menu: Play Next / Add to Queue | UI    | ✅     | Right-click song card actions                             |
| Double-click auto-queue                | UI    | ✅     | While playing, double-click queues instead of replacing   |
| Skip forward / back buttons            | UI    | ✅     | Wired to queue dequeue / history                          |
| Auto-advance on song end               | Code  | ✅     | `playback-ended` event triggers next in queue             |
| Song transition fade-in animations     | UI    | ✅     | Smooth visual transition between songs                    |

---

## Post-MVP Milestones (Future)

These milestones are scoped but not scheduled. They become relevant after v0.1.0 is released and validated.

| Milestone                      | Scope                                                                 |
| ------------------------------ | --------------------------------------------------------------------- |
| M7 — Mic Input & Vocal Effects | Microphone capture, reverb, echo, volume mix                          |
| M8b — Crossfade                | Configurable crossfade duration, dual-track rendering in audio output |
| M9 — Pitch & Key Shift         | Real-time pitch shifting of accompaniment track                       |
| M10 — Session Recording        | Record user's vocal performance, export as audio                      |
| M11 — Multi-screen             | Second display for audience lyrics view                               |
| M12 — CJK Transliteration      | Romaji/Pinyin display alongside original lyrics                       |

---

## How to Use This Document

1. **Project manager**: Assign owners to tasks in the current milestone. Track status (☐ → ⏳ → ✅).
2. **New engineer**: Find the current milestone, read its Exit Criteria, then pick an unowned task.
3. **Reviewer**: Check Exit Criteria before signing off on a milestone.
4. **Stakeholder**: Read the Demo column to understand what each milestone delivers.
