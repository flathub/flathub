# macOS Runtime Acceleration Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make Apple Silicon separation prefer a well-configured CoreML path by default, keep the settings UI to `CPU` and `CoreML`, and ensure provider changes actually take effect on the next separation.

**Architecture:** Remove `auto` from the persisted execution-provider model entirely, and migrate any legacy `execution_provider="auto"` config to the current unset/`None` semantics at load time. When config does not yet contain an execution provider, the app should choose a platform default directly, return only explicit providers to the frontend, and build sessions through an internal provider chain keyed by the effective provider plus `openkara.model_cache_key` when present. CoreML-specific session options should enable static-shape specialization, metadata-aware compiled-model caching in the app data directory, and disable duplicate runtime graph optimization for models tagged `openkara.optimized_by=onnxruntime`.

**Tech Stack:** Rust (`tauri`, `ort`), TypeScript/React, Zustand, Vitest, Cargo tests

---

### Task 1: Shrink the exposed macOS execution-provider surface

**Files:**

- Modify: `src-tauri/src/config.rs`
- Modify: `src-tauri/src/commands/settings.rs`
- Modify: `src/types/ipc.ts`
- Modify: `src/stores/settings-store.ts`
- Modify: `src/components/Settings/SettingsExecutionProviderSection.tsx`
- Modify: `src/components/Settings/SettingsOverlay.state.ts`
- Modify: `src/components/Settings/SettingsOverlay.controller.test.ts`
- Modify: `src/components/Settings/SettingsOverlay.test.tsx`
- Modify: `src/locales/en.json`
- Modify: `src/locales/zh-CN.json`
- Test: `src/components/Settings/SettingsOverlay.controller.test.ts`
- Test: `src/components/Settings/SettingsOverlay.test.tsx`

**Step 1: Write the failing tests**

Add tests that prove:

- `get_settings`-driven frontend state on macOS no longer hydrates any `auto` execution-provider value.
- The settings UI renders only the `CPU` and `CoreML` options for the macOS context fixture.
- When execution provider is unset in config, the backend resolves to an explicit platform default before hydrating the UI.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/components/Settings/SettingsOverlay.controller.test.ts src/components/Settings/SettingsOverlay.test.tsx`

Expected: FAIL because the current snapshots and labels still expose `auto` semantics.

**Step 3: Write minimal implementation**

Implement the smallest change set that:

- returns explicit resolved providers from `AppSettings`,
- removes `auto` from the execution-provider model entirely,
- hides any non-explicit provider from `available_execution_providers` on Apple,
- removes `auto` labels from the macOS settings UI copy.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/components/Settings/SettingsOverlay.controller.test.ts src/components/Settings/SettingsOverlay.test.tsx`

Expected: PASS.

**Step 5: Commit**

```bash
git add src-tauri/src/config.rs src-tauri/src/commands/settings.rs src/types/ipc.ts src/stores/settings-store.ts src/components/Settings/SettingsExecutionProviderSection.tsx src/components/Settings/SettingsOverlay.state.ts src/components/Settings/SettingsOverlay.controller.test.ts src/components/Settings/SettingsOverlay.test.tsx src/locales/en.json src/locales/zh-CN.json
git commit -m "feat(settings): internalize automatic provider selection"
```

### Task 2: Make runtime provider selection explicit and cache-safe

**Files:**

- Modify: `src-tauri/src/separator/model.rs`
- Modify: `src-tauri/src/separator/model_cache.rs`
- Modify: `src-tauri/src/separator/job.rs`
- Modify: `src-tauri/tests/phase3_model.rs`
- Test: `src-tauri/tests/phase3_model.rs`

**Step 1: Write the failing tests**

Add tests that prove:

- provider resolution turns an unset config value into `CoreML` on Apple Silicon and `CPU` on Intel macOS,
- model cache differentiates the same model path loaded with different providers,
- CoreML cache directory path generation is stable and app-data-rooted.

**Step 2: Run tests to verify they fail**

Run: `cd src-tauri && cargo test -q phase3_model`

Expected: FAIL because provider resolution helpers and cache keying do not yet exist.

**Step 3: Write minimal implementation**

Implement:

- an internal provider chain helper for macOS (`CoreML -> CPU`),
- cache keys that include the effective provider,
- explicit CoreML session builder options for static input shapes, MLProgram format, and compiled-model cache directory.

Do not add new frontend behavior in this task.

**Step 4: Run tests to verify they pass**

Run: `cd src-tauri && cargo test -q phase3_model`

Expected: PASS.

**Step 5: Commit**

```bash
git add src-tauri/src/separator/model.rs src-tauri/src/separator/model_cache.rs src-tauri/src/separator/job.rs src-tauri/tests/phase3_model.rs
git commit -m "feat(separator): harden macos provider resolution and cache semantics"
```

### Task 3: Surface runtime diagnostics without changing user flow

**Files:**

- Modify: `src-tauri/src/separator/model.rs`
- Modify: `docs/design-docs/architecture.md`
- Test: `src-tauri/tests/phase3_model.rs`

**Step 1: Write the failing test**

Add a focused test for any new helper that reports the effective provider / fallback path chosen by session setup.

**Step 2: Run test to verify it fails**

Run: `cd src-tauri && cargo test -q phase3_model`

Expected: FAIL because the diagnostic helper/output contract does not exist yet.

**Step 3: Write minimal implementation**

Add concise diagnostics around provider selection and fallback, and update the architecture doc so it no longer claims a visible `auto` option while still documenting default provider behavior.

**Step 4: Run test to verify it passes**

Run: `cd src-tauri && cargo test -q phase3_model`

Expected: PASS.

**Step 5: Commit**

```bash
git add src-tauri/src/separator/model.rs docs/design-docs/architecture.md src-tauri/tests/phase3_model.rs
git commit -m "docs(separator): document macos provider defaults and diagnostics"
```

### Task 4: Run required verification for the OpenKara change set

**Files:**

- Modify: none
- Test: project verification commands only

**Step 1: Run formatting**

Run: `pnpm format`

Expected: PASS.

**Step 2: Run frontend verification**

Run: `pnpm lint && pnpm build && pnpm test`

Expected: PASS.

**Step 3: Run backend verification**

Run: `cd src-tauri && cargo test -q`

Expected: PASS.

**Step 4: If a release-sensitive path changed, run the stricter gate**

Run: `pnpm tauri build`

Expected: PASS if runtime packaging semantics were touched enough to justify it.

**Step 5: Commit verification-only follow-ups if needed**

```bash
git add .
git commit -m "chore: finalize macos runtime acceleration verification"
```
