# macOS AppKit Container Shell Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Move the macOS shell toward official AppKit containers by first extracting the frontend state that must survive a future split-webview AppKit sidebar/container architecture.

**Architecture:** A true AppKit sidebar container in this repo requires separate sidebar and main webviews on macOS, because the current single Tauri webview owns both panes. Before creating those panes, extract the minimum shared state/actions that would otherwise break across webview boundaries: queue state, playback entry actions, and shell/sidebar visibility signals. After that, move the macOS shell container to AppKit while keeping React components as the shared business/UI layer.

**Tech Stack:** Tauri v2, Rust, AppKit/Objective-C bridge, React 19, TypeScript, Tailwind v4, Vitest

---

### Task 1: Lock in the split-webview architecture boundary with tests and docs

**Files:**

- Modify: `src/lib/window-shell.test.ts`
- Modify: `src-tauri/src/window_shell.rs`
- Modify: `docs/references/contracts/window-shell-contract.md`

**Step 1: Write the failing tests**

Add focused tests that prove a true AppKit sidebar container requires separate webview contexts and therefore needs shared-state extraction before the native container work can begin.

**Step 2: Run test to verify it fails**

Run: `pnpm test src/lib/window-shell.test.ts && cd src-tauri && cargo test -q window_shell`
Expected: FAIL because the shell model does not yet expose the stricter AppKit-only container contract.

**Step 3: Write minimal implementation**

Keep `WindowShellState` honest about the current boundary and update contract docs to state that future AppKit sidebar/container work depends on split webviews plus explicit shared-state synchronization.

**Step 4: Run test to verify it passes**

Run: `pnpm test src/lib/window-shell.test.ts && cd src-tauri && cargo test -q window_shell`
Expected: PASS.

### Task 2: Extract the first cross-webview shared state with TDD

**Files:**

- Create: `src/runtime/webview-sync.ts`
- Modify: `src/stores/queue-store.ts`
- Modify: `src/stores/player-store.ts`
- Modify: `src/main.tsx`
- Test: `src/runtime/webview-sync.test.ts`
- Test: `src/stores/queue-store.test.ts`

**Step 1: Write the failing tests**

Add tests that expect queue mutations and playback entry intents to be synchronizable across multiple webview contexts without duplicating business logic.

**Step 2: Run test to verify it fails**

Run: `pnpm test src/runtime/webview-sync.test.ts src/stores/queue-store.test.ts`
Expected: FAIL because there is no cross-webview synchronization layer yet.

**Step 3: Write minimal implementation**

Create a minimal shared-state/event layer that works across webview contexts and wire the queue store through it first. Keep the business logic in shared TS modules; only the transport becomes cross-webview aware.

**Step 4: Run test to verify it passes**

Run: `pnpm test src/runtime/webview-sync.test.ts src/stores/queue-store.test.ts`
Expected: PASS.

### Task 3: Introduce shell-mode-aware React entrypoints for future split panes

**Files:**

- Modify: `src/main.tsx`
- Modify: `src/App.tsx`
- Create: `src/components/Layout/SidebarWebviewApp.tsx`
- Create: `src/components/Layout/MainWebviewApp.tsx`
- Test: `src/App.test.tsx`

**Step 1: Write the failing tests**

Add tests that expect the app to render different shell subsets based on the current webview label or shell mode while continuing to share the same business modules.

**Step 2: Run test to verify it fails**

Run: `pnpm test src/App.test.tsx`
Expected: FAIL because the app still assumes one full-shell webview.

**Step 3: Write minimal implementation**

Refactor the React entrypoint so a future macOS sidebar child webview can render only `Sidebar`, and the main webview can render the rest of the app content, while Windows/Linux keep the current single-webview path.

**Step 4: Run test to verify it passes**

Run: `pnpm test src/App.test.tsx`
Expected: PASS.

### Task 4: Add native AppKit child-webview container scaffolding on macOS

**Files:**

- Modify: `src-tauri/Cargo.toml`
- Modify: `src-tauri/src/window_shell.rs`
- Modify: `src-tauri/src/macos/window_shell.m`
- Modify: `src-tauri/src/app_runtime.rs`
- Test: `src-tauri/src/window_shell.rs`

**Step 1: Write the failing tests**

Add tests that assert macOS shell setup can model a split-pane shell with child webview placeholders and native sidebar collapse semantics behind `#[cfg(target_os = "macos")]`.

**Step 2: Run test to verify it fails**

Run: `cd src-tauri && cargo test -q window_shell`
Expected: FAIL because native split/container scaffolding does not exist yet.

**Step 3: Write minimal implementation**

Enable the minimum Tauri APIs needed for child webviews, create the macOS-only AppKit split/container scaffolding, and keep it isolated from Windows/Linux paths.

**Step 4: Run test to verify it passes**

Run: `cd src-tauri && cargo test -q window_shell`
Expected: PASS.

### Task 5: Swap the macOS shell container to native AppKit and verify

**Files:**

- Modify: `docs/internal/project-structure.md`

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

Expected: all commands pass.

**Step 2: Update docs**

Document the final architecture boundary: AppKit owns toolbar/titlebar/container semantics on macOS, while sidebar content and business flows remain in the shared webview.

**Step 3: Report residual limits**

State explicitly that a true AppKit sidebar container or AppKit `NSOutlineView` source list would require either splitting the webview or moving sidebar content out of React.
