# Shell frontend boundary

This document freezes the split between **shared product UI** and **platform window host** so new work does not reintroduce duplicate frontend hosts or asymmetric runtimes.

## Single frontend path

- **Owner:** React `AppLayout` in [`src/components/Layout/AppLayout.tsx`](../src/components/Layout/AppLayout.tsx) is the only product shell.
- **Invariant:** There is no second entry tree for `sidebar-webview` / `main-content-webview`; those modes are not part of the product surface.
- **Runtime:** All listeners that affect product behavior run in the same process and the same hook graph: `useAppRuntime` in [`src/runtime/app-runtime.ts`](../../src/runtime/app-runtime.ts).

## Host layer (macOS)

- **Owner:** Tauri + AppKit bridge under `src-tauri/`, including [`window_shell.m`](../../src-tauri/src/macos/window_shell.m).
- **Responsibility:** System window controls (traffic lights), titlebar semantics, and window style flags needed for native macOS chrome.
- **Non-goals:** Splitting the product into multiple WKWebViews, mounting NSSplitView content hosts for sidebar content, or driving sidebar visibility via a separate native split container.

## Metrics vs. product

- `WindowShellStateSnapshot` / CSS `--window-shell-*` may still vary by tier for **leading inset**, toolbar height, and sidebar width tokens. Those are **layout metrics**, not separate product implementations.

## What not to add

- No new `shellMode` branches in product components.
- No feature that only works on a special macOS tier. Host-only behavior must be documented in [`window-shell-contract.md`](./contracts/window-shell-contract.md).
