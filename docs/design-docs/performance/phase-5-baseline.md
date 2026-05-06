# Phase 5 Performance Baseline

Current backend performance baseline for `Phase 5.3`.

## Scope

This report is generated from the Rust-side benchmark helper in `src-tauri/tests/phase5_perf.rs`.

It currently measures:

- DB lookup, metadata probe, and full decode latency for the playback load path
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
    "track_db_lookup_latency_ms": 0.1094,
    "track_metadata_probe_latency_ms": 3.8141,
    "track_full_decode_latency_ms": 64.5005,
    "seek_latency_avg_ms": 0.00008787499999999995,
    "seek_latency_p95_ms": 0.000125,
    "seek_latency_max_ms": 0.000292,
    "seek_samples": 128
  },
  "lyrics_sync": {
    "position_event_interval_ms": 33,
    "jitter_budget_ms": 33
  }
}
```

## Threshold check

- `track_db_lookup_latency_ms < 25`
- `track_metadata_probe_latency_ms < 100`
- `track_full_decode_latency_ms < 1000`
- `seek_latency_max_ms < 200`
- `lyrics_jitter_budget_ms < 50`

All thresholds pass on the current baseline.

## Notes

- `lyrics_jitter_budget_ms` is the backend-side raw timing budget, not the final UI-visible jitter. The frontend sync loop can only improve on top of this cadence.
- Playback still does an eager full decode before `start_track`, so the decode phase keeps a looser guardrail than the lookup and probe phases.
- The current report uses fixture media from `src-tauri/tests/fixtures/`. Re-run with real songs after merge to compare against the main-workspace `test/` corpus.
