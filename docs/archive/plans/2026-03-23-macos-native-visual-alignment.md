# macOS Native Visual Alignment Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Bring the macOS Native shell visually closer to the approved reference without creating a separate product UI or a second playback bar implementation.

**Architecture:** Keep the existing AppKit host plus shared React content structure. Limit the work to `mac_native` visual variants, layout tokens, and a small amount of native-only header chrome in the sidebar child webview; business logic, playback logic, and the main component graph remain shared.

**Tech Stack:** Tauri v2, Rust, AppKit/Objective-C bridge, React 19, TypeScript, Tailwind v4, Vitest

---

### Task 1: Rebuild the native sidebar shell as a translucent panel

**Files:**

- Modify: `src/components/Layout/SidebarWebviewApp.test.tsx`
- Modify: `src/components/Layout/Sidebar.test.tsx`
- Modify: `src/components/Library/SearchBox.test.tsx`
- Modify: `src/components/Layout/SidebarWebviewApp.tsx`
- Modify: `src/components/Layout/Sidebar.tsx`
- Modify: `src/components/Library/SearchBox.tsx`
- Modify: `src/components/Library/SongList.tsx`
- Modify: `src/components/Library/SongListItem.tsx`
- Modify: `src/styles/globals.css`

**Step 1: Write the failing tests**

Add tests that assert `mac_native` renders a split native sidebar header with separate sidebar/import buttons, the sidebar panel exposes a native variant marker, and native search/list rows pick up the tighter translucent styling hooks.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/components/Layout/SidebarWebviewApp.test.tsx src/components/Layout/Sidebar.test.tsx src/components/Library/SearchBox.test.tsx`

**Step 3: Write the minimal implementation**

Implement the native sidebar variant only:

- separate top-left buttons instead of the current combined capsule
- translucent rounded sidebar panel treatment
- lighter search field chrome
- denser, cleaner song rows with native-only visual tokens
- calmer bottom action bar that still uses the same batch-separate logic

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/components/Layout/SidebarWebviewApp.test.tsx src/components/Layout/Sidebar.test.tsx src/components/Library/SearchBox.test.tsx`

### Task 2: Add native stage ambience without changing playback logic

**Files:**

- Modify: `src/components/Layout/MainWebviewApp.test.tsx`
- Modify: `src/components/Playback/PlaybackStage.test.tsx`
- Modify: `src/components/Lyrics/LyricsPanel.test.tsx`
- Modify: `src/components/Layout/MainContentView.tsx`
- Modify: `src/components/Playback/PlaybackStage.tsx`
- Modify: `src/components/Lyrics/LyricsPanel.tsx`
- Modify: `src/styles/globals.css`

**Step 1: Write the failing tests**

Add tests that assert `mac_native` main content renders a native ambience layer and that the standard lyrics view gets the centered, layered presentation hooks needed for the reference layout.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/components/Layout/MainWebviewApp.test.tsx src/components/Playback/PlaybackStage.test.tsx src/components/Lyrics/LyricsPanel.test.tsx`

**Step 3: Write the minimal implementation**

Add a native-only stage background treatment driven by the current song cover art, plus stronger lyric hierarchy for the standard presentation. Do not change audience mode behavior or lyric timing logic.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/components/Layout/MainWebviewApp.test.tsx src/components/Playback/PlaybackStage.test.tsx src/components/Lyrics/LyricsPanel.test.tsx`

### Task 3: Keep one shared playback bar and give `mac_native` its own visual/layout variant

**Files:**

- Modify: `src/components/Player/PlaybackBar.test.tsx`
- Modify: `src/components/Player/NowPlayingInfo.test.tsx`
- Modify: `src/components/Player/PlayControls.test.tsx`
- Modify: `src/components/Player/QueueButton.test.tsx`
- Modify: `src/components/Player/playback-bar-layout.ts`
- Modify: `src/components/Player/PlaybackBar.tsx`
- Modify: `src/components/Player/NowPlayingInfo.tsx`
- Modify: `src/components/Player/PlayControls.tsx`
- Modify: `src/components/Player/QueueButton.tsx`
- Modify: `src/styles/globals.css`

**Step 1: Write the failing tests**

Add tests that prove `PlaybackBar` stays shared but exposes a `mac_native` variant with different spacing, background treatment, and button grouping while still rendering the same zones and child components.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/components/Player/PlaybackBar.test.tsx src/components/Player/NowPlayingInfo.test.tsx src/components/Player/PlayControls.test.tsx src/components/Player/QueueButton.test.tsx`

**Step 3: Write the minimal implementation**

Keep a single `PlaybackBar` implementation, but add a native variant that better matches the reference: lighter panel, stronger center focus, tighter right utilities, and better left-side now-playing presentation.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/components/Player/PlaybackBar.test.tsx src/components/Player/NowPlayingInfo.test.tsx src/components/Player/PlayControls.test.tsx src/components/Player/QueueButton.test.tsx`

### Task 4: Reconcile shell tokens and document the native visual boundary

**Files:**

- Modify: `docs/references/contracts/window-shell-contract.md`
- Modify: `src/lib/window-shell.test.ts`
- Modify: `src/lib/window-shell.ts`
- Modify: `src/styles/globals.css`

**Step 1: Write the failing tests**

Add tests that assert the window-shell helpers expose the native-only visual token values required by the sidebar, stage, and playback bar variants.

**Step 2: Run tests to verify they fail**

Run: `pnpm test src/lib/window-shell.test.ts`

**Step 3: Write the minimal implementation**

Add only the shell-level visual token helpers that the new native variants actually consume, and document that native visual alignment still runs inside the shared React UI.

**Step 4: Run tests to verify they pass**

Run: `pnpm test src/lib/window-shell.test.ts`

### Task 5: Run the full verification gate

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

State any remaining manual macOS smoke coverage still needed, especially visual checks comparing Native against the approved reference.
