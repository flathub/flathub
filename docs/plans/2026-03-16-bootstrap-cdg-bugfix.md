# Model Bootstrap And CDG Bugfix Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Stop OpenKara from re-downloading an already-installed model on every launch, and restore end-to-end CDG rendering so CD+G songs show graphics again during playback.

**Architecture:** Fix the model issue at startup-state derivation time instead of patching download behavior later in the flow: startup should resolve the active model variant once, publish the correct ready/pending snapshot, and only spawn a download worker when no verified model exists. Fix the CDG issue as an integration problem across Rust playback state, Tauri command registration, and the React fullscreen renderer: load sidecar CDG packets with the active song, expose a working frame command, and mount the fullscreen player in a code path that actually runs the sync loop.

**Tech Stack:** Rust, Tauri 2, React 19, TypeScript, Zustand, Vitest, cargo test

---

### Task 1: Lock the startup bootstrap regression with failing tests

**Files:**

- Modify: `src-tauri/tests/phase6_model_bootstrap.rs`
- Modify: `src-tauri/src/lib.rs`
- Modify: `src-tauri/src/separator/bootstrap.rs`
- Test: `src-tauri/tests/phase6_model_bootstrap.rs`

**Step 1: Write the failing test**

Add a startup-focused regression test that models a verified managed install and asserts startup marks the model as ready without scheduling a background download. Add a second test that sets the active model variant to `htdemucs_ft` and asserts startup resolves the correct descriptor and managed filename instead of always using `htdemucs.onnx`.

**Step 2: Run test to verify it fails**

Run: `cargo test --test phase6_model_bootstrap`
Expected: FAIL because the current startup code still schedules bootstrap whenever `model_path == managed_model_path`, which is also true for an already-installed managed model, and it still resolves the default descriptor during setup.

**Step 3: Write minimal implementation**

Extract a small startup helper from `src-tauri/src/lib.rs` that takes the active `ModelVariant`, computes the descriptor once, resolves managed vs development model paths, returns the initial bootstrap snapshot, and separately returns whether a bootstrap worker should start.

**Step 4: Run test to verify it passes**

Run: `cargo test --test phase6_model_bootstrap`
Expected: PASS

### Task 2: Fix runtime bootstrap initialization for real app startup

**Files:**

- Modify: `src-tauri/src/lib.rs`
- Modify: `src-tauri/src/commands/bootstrap.rs`
- Modify: `src-tauri/src/separator/bootstrap.rs`
- Test: `src-tauri/tests/phase6_model_bootstrap.rs`

**Step 1: Use the active model variant during setup**

Load the config once during app setup, derive the effective `ModelVariant`, and use `descriptor_for(variant)` for the managed path, checksum, and initial status snapshot.

**Step 2: Only spawn download work when startup says it is needed**

Replace the current `if model_path == managed_model_path` gate with an explicit boolean from the helper created in Task 1 so verified managed installs do not trigger re-downloads on app open.

**Step 3: Keep command behavior aligned with startup semantics**

Review `download_model`, `get_model_status`, and bootstrap status updates so they continue to report the same `model_path`, `state`, and variant-aware readiness that startup now uses.

**Step 4: Re-run the focused bootstrap test suite**

Run: `cargo test --test phase6_model_bootstrap`
Expected: PASS

### Task 3: Restore backend CDG lifecycle and IPC wiring

**Files:**

- Modify: `src-tauri/src/lib.rs`
- Modify: `src-tauri/src/commands/mod.rs`
- Modify: `src-tauri/src/commands/playback.rs`
- Modify: `src-tauri/src/commands/cdg.rs`
- Modify: `src-tauri/src/cdg/mod.rs`
- Modify: `src-tauri/src/cdg/parser.rs`
- Modify: `src-tauri/src/cdg/renderer.rs`
- Test: `src-tauri/src/cdg/parser.rs`
- Test: `src-tauri/src/cdg/renderer.rs`

**Step 1: Add a failing backend integration point**

Add a focused Rust test around a helper that loads CDG packets for the active track from a same-basename sidecar file and clears or resets CDG state when playback starts a non-CDG song or seeks backward.

**Step 2: Wire the CDG command into the crate**

Register the `cdg` module in `commands/mod.rs`, add CDG state storage to `AppState`, and expose `get_cdg_frame` through the Tauri invoke handler in `src-tauri/src/lib.rs`.

**Step 3: Hook CDG state into playback transitions**

Update `play()` and `seek()` so starting a track loads any available sidecar `.cdg` packets and backward seeks mark the renderer for reset before the next frame request.

**Step 4: Keep parser and renderer behavior covered**

Retain and extend the existing parser/renderer unit tests so packet parsing, palette writes, memory preset handling, and visible-frame rendering still work after the wiring changes.

**Step 5: Run backend CDG verification**

Run: `cargo test cdg`
Expected: PASS with parser, renderer, and any new CDG lifecycle tests included in the run

### Task 4: Restore frontend CDG rendering and fullscreen entry flow

**Files:**

- Modify: `src/main.tsx`
- Modify: `src/App.tsx`
- Modify: `src/lib/tauri.ts`
- Modify: `src/hooks/use-cdg-sync.ts`
- Modify: `src/stores/player-store.ts`
- Modify: `src/stores/cdg-store.ts`
- Modify: `src/components/Player/FullscreenPlayerView.tsx`
- Modify: `src/components/Cdg/CdgCanvas.tsx`
- Modify: `src/components/Layout/AppLayout.tsx`
- Test: `src/lib/tauri.ts`

**Step 1: Add a failing frontend regression check**

Add a small Vitest around the CDG IPC wrapper or a helper extracted from it so the test asserts `get_cdg_frame` is invoked with the backend’s `positionMs` argument name rather than the current mismatched payload.

**Step 2: Mount the fullscreen player intentionally**

Update `src/main.tsx` so `index.html?mode=fullscreen-player` renders `FullscreenPlayerView` instead of booting the normal library shell.

**Step 3: Run the CDG sync loop in a live code path**

Mount `useCdgSync()` in the fullscreen player flow, ensure the canvas ref is registered, and clear stale CDG state when no CDG sidecar is active.

**Step 4: Derive and consume CDG availability consistently**

Set `hasCdg` from the current song/playback transition instead of leaving it permanently false, then make the fullscreen view choose `CdgCanvas` only when that state is true.

**Step 5: Run focused frontend verification**

Run: `pnpm test`
Expected: PASS for the existing Vitest suite plus the new CDG wrapper regression

### Task 5: Verify the combined fix before finishing

**Files:**

- Modify: none
- Test: `src-tauri/tests/phase6_model_bootstrap.rs`
- Test: `src-tauri/src/cdg/parser.rs`
- Test: `src-tauri/src/cdg/renderer.rs`

**Step 1: Run required formatting gate**

Run: `pnpm format`
Expected: PASS

**Step 2: Run frontend verification**

Run: `pnpm lint && pnpm build`
Expected: PASS

**Step 3: Run backend verification**

Run: `cargo test`
Expected: PASS

**Step 4: Do one manual smoke check in the app**

Run: `pnpm tauri dev`
Expected: a previously downloaded model stays in `ready` state after relaunch, and a song with an accompanying `.cdg` sidecar renders graphics correctly in the fullscreen player.

**Step 5: Commit**

Only if explicitly requested by the user.
