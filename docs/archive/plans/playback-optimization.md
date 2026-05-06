# Playback Optimization Plan

Archived: 2026-05-05

This plan is no longer active execution guidance. It was reviewed against the
current app and split into implemented, already-satisfied, and rejected items.

## Outcome

Implemented:

- Position event cadence reduced from 16ms to 33ms.
- Position event emission now ignores tiny movement deltas.
- Backend position IPC is suppressed while the AirPlay audience output is active.
- Audio output is pre-warmed during app startup.
- Stem-volume slider IPC now uses a 20ms trailing rate limiter.
- The Phase 5 performance baseline now reflects the 33ms backend timing budget.

Already satisfied by current code:

- Seek-bar dragging does not issue repeated backend seeks. The current
  component updates local drag UI and sends one final `seek` on mouseup, which
  is cheaper than the plan's proposed throttled drag IPC.

Rejected after current-version review:

- CDG frame prefetch in `playback-position` events. It would couple CDG frame
  generation to the playback position emitter and increase work under the
  playback path. The current CDG display path is intentionally message-driven.
- Decoded PCM LRU cache. The plan's proposed 200 MB cache is a broad memory
  tradeoff without current profiling evidence that replay decode is the
  dominant user-facing bottleneck.
- `next_track` gapless hot-swap. The current playback flow has remote-load
  async work in flight and an audio callback render-frame model. A correct
  gapless implementation needs a fresh focused design rather than applying this
  point-in-time sketch.

## Notes

The original plan also described remote loading and queue preload ideas. Those
overlap with current remote-library behavior and should not be treated as a
ready-to-implement contract.
