# Current Implementation Status

> **Last updated:** 2026-04-25 · This file tracks the implementation status and is updated alongside releases.

## Completed Milestones

### ✅ v0.1 — MVP (Released)

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

### ✅ v0.2.0 — Released

OpenKara v0.2.0 is the release that established the current core app flow.

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

### ✅ v0.3.0 — Released

OpenKara v0.3.0 adds:

- [x] AirPlay support for casting playback to compatible devices
- [x] Improved player behavior and layout at narrow window widths
- [x] Visual refinements to the Windows app appearance
- [x] Better preservation of original track metadata on import
- [x] WinGet installation support on Windows

### ✅ v0.4.0 — Released

OpenKara v0.4.0 adds:

- [x] Refined macOS host chrome behavior, including tighter titlebar metrics and better traffic-light alignment
- [x] Fixed a crash that could occur after long idle/suspend periods

### ✅ v0.5.1 — Released

OpenKara v0.5.1 adds:

- [x] Upgraded separator runtime acceleration path to XNNPACK for more stable performance
- [x] Improved hardware acceleration provider selection and fallback behavior across settings and separation flow
- [x] Fixed song dialogs layering so dialogs reliably appear above list rows
- [x] Refined desktop titlebar controls placement for better usability
- [x] Includes lyrics auto-scroll behavior improvements

### ✅ v0.6.0 — Released (Current Stable)

OpenKara v0.6.0 is the current stable release. It adds:

- [x] Remote Library Support: Fully implemented connection, sync, and playback for Google Drive, Dropbox, and WebDAV providers
- [x] Secure Credential Storage: Authentication tokens are now securely stored in the system Keychain (macOS) or Credential Manager (Windows)
- [x] Legal & Privacy: Added dedicated Privacy Policy and Terms of Service disclosures

## Planned Future Features

### 🎯 v0.7 and Beyond

- **Mic Input & Vocal Effects** — Microphone capture, reverb, echo, volume mix
- **Saved Playlists & Singer Rotation** — Named playlists, singer assignment, and stronger turn-based queue workflows
- **Pitch & Key Shift** — Real-time pitch shifting of the accompaniment track
- **Session Recording** — Record vocal performances, export as audio
- **CJK Transliteration** — Romaji / Pinyin display alongside original lyrics
- **Mobile Companion App** — Remote control and lyrics display on phone/tablet

---

_For the full technical roadmap with milestones and phase planning, see [Technical Roadmap](./roadmap.md)._
