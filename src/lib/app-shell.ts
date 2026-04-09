/**
 * The product always runs as a single full-window React tree. Host-native code
 * may still use webview labels internally, but the frontend must not branch on
 * shell modes or secondary webviews.
 */
export type AppShellMode = "full-app";

export function resolveAppShellMode(
  _search = typeof window === "undefined" ? "" : window.location.search,
  _webviewLabel?: string,
): AppShellMode {
  void _search;
  void _webviewLabel;
  return "full-app";
}

export function resolveCurrentAppShellMode(): AppShellMode {
  return "full-app";
}
