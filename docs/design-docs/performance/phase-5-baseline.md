# Phase 5 Performance Baseline

Current backend performance baseline for `Phase 5.3`.

## Scope

This report is generated from the Rust-side benchmark helper in `src-tauri/tests/phase5_perf.rs`.

It currently measures:

- track load latency through `play_song_from_library`
- seek latency inside the playback controller after a real track load
- backend lyrics timing budget derived from the `playback-position` emitter cadence

## Reproduce

```bash
cd src-tauri
cargo test --test phase5_perf -- --nocapture
```

## Latest observed result

Captured on `2026-03-13` from branch `codex/phase0-m0`:

```json
{
  "playback": {
    "track_load_latency_ms": 64.5005,
    "seek_latency_avg_ms": 0.00008787499999999995,
    "seek_latency_p95_ms": 0.000125,
    "seek_latency_max_ms": 0.000292,
    "seek_samples": 128
  },
  "lyrics_sync": {
    "position_event_interval_ms": 16,
    "jitter_budget_ms": 16
  }
}
```

## Threshold check

- `track_load_latency_ms < 200`
- `seek_latency_max_ms < 200`
- `lyrics_jitter_budget_ms < 50`

All thresholds pass on the current baseline.

## Notes

- `lyrics_jitter_budget_ms` is the backend-side raw timing budget, not the final UI-visible jitter. The frontend sync loop can only improve on top of this cadence.
- The current report uses fixture media from `src-tauri/tests/fixtures/`. Re-run with real songs after merge to compare against the main-workspace `test/` corpus.
