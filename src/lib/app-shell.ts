import { getCurrentWebviewWindow } from "@tauri-apps/api/webviewWindow";

export type AppShellMode =
  | "full-app"
  | "sidebar-webview"
  | "main-content-webview";

export const SIDEBAR_CHILD_WEBVIEW_LABEL = "main-sidebar";
export const MAIN_WEBVIEW_LABEL = "main";

export function resolveAppShellMode(
  search = typeof window === "undefined" ? "" : window.location.search,
  webviewLabel?: string,
): AppShellMode {
  if (webviewLabel === SIDEBAR_CHILD_WEBVIEW_LABEL) {
    return "sidebar-webview";
  }

  const shell = new URLSearchParams(search).get("shell");

  if (shell === "sidebar-webview" || shell === "main-content-webview") {
    return shell;
  }

  return "full-app";
}

export function resolveCurrentAppShellMode(): AppShellMode {
  try {
    return resolveAppShellMode(undefined, getCurrentWebviewWindow().label);
  } catch {
    return resolveAppShellMode();
  }
}
