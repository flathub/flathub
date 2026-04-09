import { describe, expect, test } from "vitest";
import { resolveAppShellMode, resolveCurrentAppShellMode } from "./app-shell";

describe("app shell resolution", () => {
  test("ignores legacy shell query params and webview labels", () => {
    expect(resolveAppShellMode("")).toBe("full-app");
    expect(resolveAppShellMode("?shell=sidebar-webview")).toBe("full-app");
    expect(resolveAppShellMode("?shell=main-content-webview")).toBe("full-app");
    expect(resolveAppShellMode("", "main-sidebar")).toBe("full-app");
    expect(resolveAppShellMode("", "main")).toBe("full-app");
    expect(resolveAppShellMode("?shell=main-content-webview", "main")).toBe(
      "full-app",
    );
  });

  test("resolveCurrentAppShellMode matches the single product shell", () => {
    expect(resolveCurrentAppShellMode()).toBe("full-app");
  });
});
