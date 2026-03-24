import { describe, expect, test, vi } from "vitest";
import {
  MAIN_WEBVIEW_LABEL,
  resolveCurrentAppShellMode,
  SIDEBAR_CHILD_WEBVIEW_LABEL,
  resolveAppShellMode,
} from "./app-shell";

const { mockGetCurrentWebviewWindow } = vi.hoisted(() => ({
  mockGetCurrentWebviewWindow: vi.fn(() => ({
    label: "main-sidebar",
  })),
}));

vi.mock("@tauri-apps/api/webviewWindow", () => ({
  getCurrentWebviewWindow: mockGetCurrentWebviewWindow,
}));

describe("resolveAppShellMode", () => {
  test("defaults to the full app shell", () => {
    expect(resolveAppShellMode("")).toBe("full-app");
  });

  test("resolves the sidebar webview shell from the URL", () => {
    expect(resolveAppShellMode("?shell=sidebar-webview")).toBe(
      "sidebar-webview",
    );
  });

  test("resolves the main content webview shell from the URL", () => {
    expect(resolveAppShellMode("?shell=main-content-webview")).toBe(
      "main-content-webview",
    );
  });

  test("resolves the sidebar shell from the child webview label", () => {
    expect(resolveAppShellMode("", SIDEBAR_CHILD_WEBVIEW_LABEL)).toBe(
      "sidebar-webview",
    );
  });

  test("keeps the main window label on the shared full-app shell", () => {
    expect(resolveAppShellMode("", MAIN_WEBVIEW_LABEL)).toBe("full-app");
  });

  test("resolves the current app shell from the current webview label", () => {
    mockGetCurrentWebviewWindow.mockReturnValue({
      label: SIDEBAR_CHILD_WEBVIEW_LABEL,
    });

    expect(resolveCurrentAppShellMode()).toBe("sidebar-webview");
  });

  test("keeps the main window on the shared full-app shell", () => {
    mockGetCurrentWebviewWindow.mockReturnValue({ label: "main" });

    expect(resolveCurrentAppShellMode()).toBe("full-app");
  });

  test("ignores the removed toolbar shell query", () => {
    expect(resolveAppShellMode("?shell=toolbar-webview")).toBe("full-app");
  });

  test("prefers the explicit main-content shell query for the main window", () => {
    mockGetCurrentWebviewWindow.mockReturnValue({ label: "main" });

    expect(resolveAppShellMode("?shell=main-content-webview", "main")).toBe(
      "main-content-webview",
    );
  });
});
