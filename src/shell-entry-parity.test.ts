import { describe, expect, test } from "vitest";

describe("shell entry parity guards", () => {
  test("App.tsx never reintroduces secondary webview shell entry switches", async () => {
    const { default: appSource } = await import("./App.tsx?raw");
    expect(appSource).not.toContain("sidebar-webview");
    expect(appSource).not.toContain("main-content-webview");
    expect(appSource).not.toContain("MainWebviewApp");
    expect(appSource).not.toContain("SidebarWebviewApp");
    expect(appSource).toContain("useAppRuntime");
    expect(appSource).not.toContain("useMainWindowRuntimeWhen");
  });

  test("app-shell resolves only the unified full-app product mode", async () => {
    const { default: shellSource } = await import("./lib/app-shell.ts?raw");
    expect(shellSource).not.toContain('"sidebar-webview"');
    expect(shellSource).not.toContain('"main-content-webview"');
  });
});
