# lyrics

Lyrics backend for Phase 4.

Current scope:

- LRCLIB and LrcApi clients for synced lyric lookup
- LRC parser for timestamped lyric lines
- Fetch priority chain: LRCLIB -> LrcApi -> embedded tags -> sidecar `.lrc`
- SQLite lyrics cache and per-song offset persistence
- Command-facing payloads for `fetch_lyrics`, `fetch_lyrics_online`, and `set_lyrics_offset`

Follow-up work outside this directory:

- React lyrics panel and sync loop handled by the UI agent
- End-to-end validation with playback/separation in Phase 5
