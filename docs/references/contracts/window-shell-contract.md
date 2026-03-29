# Window Shell Contract

This contract documents the backend-facing shell snapshot used by the shared
React layout to adapt host chrome and container assembly without turning macOS
into a separate product UI.

## Commands

- `get_window_shell_state() -> WindowShellStateSnapshot`
- `set_native_sidebar_visibility(visible: boolean) -> void`
- `set_macos_shell_mode(mode: "stable" | "native") -> AppSettings`
- `restart_app() -> void`

## Payload

```ts
type WindowShellChromeVariant = "desktop" | "mac";
type WindowShellTier = "desktop" | "mac_legacy" | "mac_native";

interface WindowShellStateSnapshot {
  chrome_variant: WindowShellChromeVariant;
  tier: WindowShellTier;
  toolbar_height: number;
  traffic_light_inset_leading: number;
  sidebar_header_height: number;
  sidebar_width: number;
  sidebar_webview_label?: string | null;
  main_content_webview_label?: string | null;
}
```

## Semantics

- `chrome_variant`
  - `desktop`: existing Windows/Linux custom titlebar path
  - `mac`: macOS shell metrics and chrome tokens
- `tier`
  - `desktop`: no mac-specific shell treatment
  - `mac_legacy`: macOS Stable path; shared single-webview app with mac shell metrics
  - `mac_native`: macOS Native path; AppKit container host plus shared React content
- `toolbar_height`
  - Native toolbar/titlebar height mirrored into CSS tokens
- `traffic_light_inset_leading`
  - Leading inset reserved for standard window controls
- `sidebar_header_height`
  - Reserved native sidebar header strip height so search/list content starts below the system traffic lights instead of competing with them
- `sidebar_width`
  - Shared shell width token for sidebar rail/layout alignment across all platforms
- `sidebar_webview_label`
  - macOS Native sidebar child webview label; omitted for Stable and non-macOS paths
- `main_content_webview_label`
  - Label for the webview that hosts the shared main content in macOS Native mode

## Shared UI Boundary

- Library, playback, lyrics, settings, and shared layout components stay in the shared React/Tailwind UI.
- `WindowShellStateSnapshot` only controls host assembly and visual metrics:
  - toolbar/titlebar metrics
  - reserved leading inset for traffic lights
  - reserved native sidebar header height for traffic-light-safe sidebar composition
  - sidebar width tokens
  - Native-only child webview/container wiring
- Windows/Linux do not need to understand AppKit container details.
- macOS Stable and macOS Native must continue to render the same product UI modules; Native only changes the host/container behavior.
- Native visual alignment is still implemented inside the shared React UI. Sidebar, lyrics stage, and playback bar may consume `mac_native` layout and visual tokens, but they must remain shared components rather than separate product-specific implementations.
- In `mac_native`, AppKit owns the sidebar material surface and divider. The sidebar child webview is transparent and renders content overlays only; it must not repaint a full sidebar background.
- `mac_native` should not depend on a separate AppKit toolbar layer for its visible controls. The split container owns the full-height sidebar region, while native-only top-right utility controls live in the shared React content overlay.

## Shell Style Selection

- macOS exposes exactly two persisted shell style values: `stable` and `native`.
- The backend must reject any other shell style values.
- Switching shell style updates the persisted target mode only; the current app process keeps its existing host assembly until restart.
- `restart_app` is the supported apply path for shell-style changes; this contract does not support live host hot-switching.

## Sidebar Visibility Sync

- `set_native_sidebar_visibility`
  - macOS-only native container sync path
  - Mirrors the shared frontend sidebar visibility state back into the AppKit split container
  - Safe no-op on platforms or shell tiers without the native split container

## Stability Rules

- Windows/Linux must keep returning `desktop` values unless their shell design is intentionally changed.
- macOS Stable must keep using the shared single-webview UI path.
- macOS Native setup may fall back to `mac_legacy`, but it must never report `mac_native` if AppKit setup fails.
- Frontend code must treat unknown/missing values as a signal to fall back to existing desktop or legacy tokens rather than inventing a new shell mode.
- Future frontend visual work should primarily happen in shared components and CSS tokens, so one change benefits all platforms while shell hosts remain platform-specific.
