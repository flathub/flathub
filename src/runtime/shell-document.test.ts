// @vitest-environment jsdom

import { beforeEach, describe, expect, test, vi } from "vitest";
import { applyShellDocumentMarker } from "./shell-document";

const { mockResolveCurrentAppShellMode } = vi.hoisted(() => ({
  mockResolveCurrentAppShellMode: vi.fn(),
}));

vi.mock("@/lib/app-shell", () => ({
  resolveCurrentAppShellMode: mockResolveCurrentAppShellMode,
}));

describe("applyShellDocumentMarker", () => {
  beforeEach(() => {
    document.documentElement.removeAttribute("data-app-shell");
    document.body.innerHTML = '<div id="root"></div>';
    document.body.removeAttribute("data-app-shell");
    mockResolveCurrentAppShellMode.mockReset();
  });

  test("marks html body and root for the sidebar child webview", () => {
    mockResolveCurrentAppShellMode.mockReturnValue("sidebar-webview");

    applyShellDocumentMarker();

    expect(document.documentElement.dataset.appShell).toBe("sidebar-webview");
    expect(document.body.dataset.appShell).toBe("sidebar-webview");
    expect(document.getElementById("root")?.dataset.appShell).toBe(
      "sidebar-webview",
    );
  });

  test("leaves full-app and main-content-webview opaque", () => {
    mockResolveCurrentAppShellMode.mockReturnValue("main-content-webview");

    applyShellDocumentMarker();

    expect(document.documentElement.dataset.appShell).toBe(
      "main-content-webview",
    );
    expect(document.body.dataset.appShell).toBe("main-content-webview");
    expect(document.getElementById("root")?.dataset.appShell).toBe(
      "main-content-webview",
    );

    mockResolveCurrentAppShellMode.mockReturnValue("full-app");

    applyShellDocumentMarker();

    expect(document.documentElement.dataset.appShell).toBe("full-app");
    expect(document.body.dataset.appShell).toBe("full-app");
    expect(document.getElementById("root")?.dataset.appShell).toBe("full-app");
  });
});
