# macOS Native Material Sidebar Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make `mac_native` render the left sidebar as true AppKit material, with the native host owning the background and the sidebar child webview rendering content only.

**Architecture:** Start from first principles: the visual seam exists because two layers both claim ownership of the same surface. The fix is not more offsets, fallbacks, or compensation code. The fix is one clear owner per concern: AppKit owns the sidebar material and divider; the sidebar child `WKWebView` owns only interactive content. Keep the current split-webview host architecture, remove redundant React background painting in the native sidebar path, and delete obsolete background tokens/classes once the transparent-content path is verified.

**Tech Stack:** Tauri v2, Rust, Objective-C/AppKit/WebKit, React 19, TypeScript, Tailwind v4, Vitest

---

## Implementation Rules

- Use first-principles reasoning throughout: each visual layer must have exactly one owner.
- Do not add fallback paths, compatibility shims, or defensive branching for hypothetical future failures.
- Do not keep old background code alive behind conditionals once the new ownership model is verified.
- Clean up obsolete CSS tokens, classes, and comments in the same change once the new path is confirmed safe.
- Limit the change to `mac_native`; Stable and non-macOS paths should remain behaviorally unchanged.
- Do not try to fake subpixel antialiasing. Transparent webviews will use grayscale smoothing; only tune text weight/contrast if the rendered result actually needs it.

### Task 1: Mark the sidebar child document as a transparent host and keep every other shell opaque

**Files:**

- Create: `src/runtime/shell-document.ts`
- Create: `src/runtime/shell-document.test.ts`
- Modify: `src/main.tsx`
- Modify: `src/styles/globals.css`

**Step 1: Write the failing test**

Add tests for a tiny document-host helper that:

- marks `html`, `body`, and `#root` with `data-app-shell="sidebar-webview"` when the current shell is the sidebar child webview
- does not mark `full-app` or `main-content-webview` as transparent hosts

**Step 2: Run the test to verify it fails**

Run:

```bash
pnpm test src/runtime/shell-document.test.ts
```

**Step 3: Write the minimal implementation**

- Add `src/runtime/shell-document.ts` with one small function that reads the current shell mode and writes a stable `data-app-shell` marker to `document.documentElement`, `document.body`, and `#root`
- Call that helper in `src/main.tsx` before React mounts
- In `src/styles/globals.css`, make only the sidebar child path transparent:
  - `html[data-app-shell="sidebar-webview"]`
  - `body[data-app-shell="sidebar-webview"]`
  - `#root[data-app-shell="sidebar-webview"]`
- Do not make global root styles transparent; that would incorrectly affect Stable and main-content shells

**Step 4: Run the test to verify it passes**

Run:

```bash
pnpm test src/runtime/shell-document.test.ts
```

### Task 2: Remove React ownership of the native sidebar background and keep only content surfaces

**Files:**

- Modify: `src/components/Layout/SidebarWebviewApp.tsx`
- Modify: `src/components/Layout/SidebarWebviewApp.test.tsx`
- Modify: `src/components/Layout/Sidebar.tsx`
- Modify: `src/components/Layout/Sidebar.test.tsx`
- Modify: `src/components/Library/SearchBox.tsx`
- Modify: `src/components/Library/SearchBox.test.tsx`
- Modify: `src/components/Library/SongListItem.tsx`
- Modify: `src/components/Library/SongListItem.test.tsx`
- Modify: `src/styles/globals.css`

**Step 1: Write the failing tests**

Update tests to assert that in `mac_native`:

- `SidebarWebviewApp` exposes a native transparent-host marker
- `Sidebar` native variant no longer renders a full sidebar background surface
- native search and list rows use low-alpha overlay hooks rather than opaque blocks
- native selected rows and action controls still render distinctly, but as content overlays above material rather than replacement panels

**Step 2: Run the tests to verify they fail**

Run:

```bash
pnpm test src/components/Layout/SidebarWebviewApp.test.tsx src/components/Layout/Sidebar.test.tsx src/components/Library/SearchBox.test.tsx src/components/Library/SongListItem.test.tsx
```

**Step 3: Write the minimal implementation**

- In `src/components/Layout/SidebarWebviewApp.tsx`, keep the native header structure but make the root host explicitly content-only for `mac_native`
- In `src/components/Layout/Sidebar.tsx`, remove native-mode ownership of the sidebar panel background; keep layout, borders, spacing, and content composition only
- In `src/components/Library/SearchBox.tsx`, keep the control chrome but express it as a low-alpha overlay suitable for material underneath
- In `src/components/Library/SongListItem.tsx`, keep hover, selected, badge, and trailing actions readable while converting opaque fills to alpha-tinted overlays that still reveal native material
- In `src/styles/globals.css`, introduce only the tokens actually needed for this content-over-material model, such as:
  - `--native-sidebar-overlay-bg`
  - `--native-sidebar-selected-bg`
  - `--native-sidebar-selected-border`
  - `--native-sidebar-control-bg`
- Remove the old native sidebar background token from actual use in the sidebar path once the new styling is in place

**Step 4: Run the tests to verify they pass**

Run:

```bash
pnpm test src/components/Layout/SidebarWebviewApp.test.tsx src/components/Layout/Sidebar.test.tsx src/components/Library/SearchBox.test.tsx src/components/Library/SongListItem.test.tsx
```

### Task 3: Make the sidebar child `WKWebView` transparent and leave the main content webview opaque

**Files:**

- Modify: `src-tauri/src/macos/window_shell.m`

**Step 1: Implement the native webview ownership change**

In `src-tauri/src/macos/window_shell.m`, add one small helper for configuring a `WKWebView` as transparent or opaque, then call it during split-shell mounting:

- set the sidebar child `WKWebView` to transparent
- keep the main content `WKWebView` opaque
- if needed, clear the pane controller view backgrounds so the AppKit material remains visible through the sidebar child host

Use direct WebKit/AppKit properties only. Do not add a Rust-side config layer for this; the ownership change belongs entirely in the macOS bridge.

**Step 2: Run targeted native verification**

Run:

```bash
cd src-tauri && cargo test -q window_shell
```

**Step 3: Run a real bridge compile**

Run from repo root:

```bash
pnpm tauri build --debug --no-bundle --ci
```

**Step 4: Perform manual smoke verification on macOS**

Run:

```bash
pnpm tauri dev
```

Verify all of the following in `mac_native`:

- the traffic lights sit visually inside the same left material surface as the sidebar
- there is no separate dark titlebar band above the sidebar content
- the sidebar search box, list rows, and actions float above native material instead of repainting a full panel
- sidebar collapse/expand keeps the material, divider, and content aligned
- text clarity is acceptable with transparent webview rendering

### Task 4: Tune text rendering only if the transparent sidebar actually needs it

**Files:**

- Modify: `src/styles/globals.css`
- Modify: `src/components/Library/SearchBox.tsx`
- Modify: `src/components/Library/SongListItem.tsx`

**Step 1: Inspect the live result before changing text styling**

Do not preemptively add font hacks. After Task 3 smoke verification, decide whether text in the native sidebar is materially too thin or too soft.

**Step 2: If the live result needs it, apply the smallest possible adjustment**

Allowed adjustments only:

- slightly stronger text color contrast for native sidebar content
- a small font-weight increase on the specific native sidebar text that looks weak

Do not add broad text-rendering experiments, browser-specific font overrides, or multiple fallback smoothing strategies.

**Step 3: Re-run the relevant frontend tests**

Run:

```bash
pnpm test src/components/Library/SearchBox.test.tsx src/components/Library/SongListItem.test.tsx src/components/Layout/Sidebar.test.tsx
```

### Task 5: Delete obsolete native-sidebar background code after the new ownership model is verified

**Files:**

- Modify: `src/components/Layout/Sidebar.tsx`
- Modify: `src/styles/globals.css`
- Modify: `docs/references/contracts/window-shell-contract.md`

**Step 1: Remove dead styling and stale rationale**

Once Tasks 2 and 3 are verified, delete any code that still assumes React paints the native sidebar background:

- unused `--native-sidebar-bg` token definitions
- unused sidebar panel classes tied to CSS blur or full-surface painting in `mac_native`
- stale comments describing the old mixed-ownership model

Keep only code that is still exercised by the final `mac_native` design.

**Step 2: Update the contract**

Document the new boundary in `docs/references/contracts/window-shell-contract.md`:

- AppKit owns the native sidebar material and divider in `mac_native`
- the sidebar child webview is transparent and renders content only
- Stable and non-macOS paths remain on their current ownership model

**Step 3: Re-run the focused tests**

Run:

```bash
pnpm test src/components/Layout/SidebarWebviewApp.test.tsx src/components/Layout/Sidebar.test.tsx src/components/Library/SearchBox.test.tsx src/components/Library/SongListItem.test.tsx
```

### Task 6: Run the full verification gate

**Files:**

- Verify only

**Step 1: Run formatting and required verification**

Run from repo root:

```bash
pnpm format
pnpm lint
pnpm build
pnpm test
cd src-tauri && cargo test -q
cd .. && pnpm tauri build
```

**Step 2: Perform final manual smoke coverage**

Verify:

- macOS Stable is visually unchanged
- macOS Native now shows one continuous left material surface from traffic lights through the sidebar content
- hover, selection, search focus, and action controls still feel intentional over material
- no stale React sidebar background remains visible in `mac_native`
- the main content webview remains opaque and unaffected

**Step 3: Report residual limits honestly**

If any issue remains, report the exact visual or rendering artifact instead of leaving dead code or dormant fallback branches in place.
