# macOS Shared Native Shell Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Tighten the macOS AppKit host path without turning macOS into a separate product UI, so most frontend changes remain shared across platforms and macOS users can switch between Stable and Native shell styles.

**Architecture:** Keep one shared React/Tailwind UI for library, playback, lyrics, settings, and layout components. Platform-specific work stays in the shell host layer only: Windows/Linux keep the existing single-webview path, macOS Stable keeps the shared single-webview shell with mac metrics, and macOS Native adds the AppKit split container plus sidebar child webview while reusing the same shared UI modules.

**Tech Stack:** Tauri v2, Rust, AppKit/Objective-C bridge, React 19, TypeScript, Tailwind v4, Vitest

---

### Task 1: Align the contract with the shared-UI plus Stable/Native host strategy

**Files:**

- Modify: `docs/references/contracts/window-shell-contract.md`
- Modify: `docs/plans/2026-03-22-macos-shared-native-shell.md`

**Step 1: Update the contract**

Document that shell state only controls host assembly and visual metrics, not product-level business UI splits.

**Step 2: Verify the contract matches the target behavior**

Check that the doc now states:

- Windows/Linux keep the shared single-webview app
- macOS Stable uses the shared app with mac shell metrics
- macOS Native uses an AppKit container while reusing the shared React content

### Task 2: Tighten shell mode handling with tests first

**Files:**

- Modify: `src/lib/app-shell.test.ts`
- Modify: `src/App.test.tsx`
- Modify: `src/lib/app-shell.ts`
- Modify: `src/App.tsx`
- Delete or stop using: `src/components/Layout/ToolbarWebviewApp.tsx`

**Step 1: Write the failing tests**

Add tests that prove only the real host paths remain active: full app, sidebar webview, and main-content webview. Remove the obsolete toolbar-only shell path.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/lib/app-shell.test.ts src/App.test.tsx`

**Step 3: Write the minimal implementation**

Keep shell modes aligned with the real macOS AppKit host path and ensure the main webview remains the shared app or the shared main-content host depending on shell query, never a separate toolbar app.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/lib/app-shell.test.ts src/App.test.tsx`

### Task 3: Rename the macOS visual mode to Stable/Native and preserve shared behavior

**Files:**

- Modify: `src/types/ipc.ts`
- Modify: `src/stores/settings-store.ts`
- Modify: `src/stores/settings-store.test.ts`
- Modify: `src/components/Settings/SettingsGeneralSection.tsx`
- Modify: `src/components/Settings/SettingsOverlay.state.ts`
- Modify: `src/components/Settings/SettingsOverlay.controller.test.ts`
- Modify: `src/runtime/app-runtime.test.ts`
- Modify: `src/locales/en.json`
- Modify: `src/locales/zh-CN.json`
- Modify: `src-tauri/src/config.rs`
- Modify: `src-tauri/src/commands/settings.rs`

**Step 1: Write the failing tests**

Update tests to expect the user-facing macOS shell mode to be `stable | native` while keeping the same shared settings shape and restart semantics.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/stores/settings-store.test.ts src/components/Settings/SettingsOverlay.controller.test.ts src/runtime/app-runtime.test.ts && cd src-tauri && cargo test -q config::tests commands::settings::tests`

**Step 3: Write the minimal implementation**

Rename the mode consistently, keep backward-compatible config loading if older values exist, and keep Native as a host-style choice rather than a separate app path.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/stores/settings-store.test.ts src/components/Settings/SettingsOverlay.controller.test.ts src/runtime/app-runtime.test.ts && cd src-tauri && cargo test -q config::tests commands::settings::tests`

### Task 4: Tighten the AppKit host path and labels without splitting shared UI

**Files:**

- Modify: `src/lib/window-shell.ts`
- Modify: `src/runtime/native-shell-runtime.ts`
- Modify: `src-tauri/src/window_shell.rs`

**Step 1: Write the failing tests**

Add tests that assert Native reports only the real host labels and that Stable never exposes native child-webview labels.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/lib/window-shell.test.ts && cd src-tauri && cargo test -q window_shell`

**Step 3: Write the minimal implementation**

Make the main content host label and shell parsing agree with the actual runtime, and keep AppKit fallback strict so Stable remains the safe path.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/lib/window-shell.test.ts && cd src-tauri && cargo test -q window_shell`

### Task 5: Run the full cross-stack verification gate

**Files:**

- Verify only

**Step 1: Run formatting and verification**

Run from repo root:

```bash
pnpm format
pnpm lint
pnpm build
pnpm test
cd src-tauri && cargo test -q
cd .. && pnpm tauri build
```

**Step 2: Report residual limits**

State any remaining manual macOS smoke coverage still needed, especially around live AppKit shell switching versus restart-required behavior.
