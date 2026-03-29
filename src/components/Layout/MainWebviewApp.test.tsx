import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import type { WindowShellState } from "@/lib/window-shell";
import { MainWebviewApp } from "./MainWebviewApp";

vi.mock("@/lib/app-shortcuts", () => ({
  getShortcutPlatform: () => "mac",
}));

describe("MainWebviewApp", () => {
  test("defaults child main-content hosts to native shell tokens before IPC hydration", () => {
    const markup = renderToStaticMarkup(<MainWebviewApp />);

    expect(markup).toContain('data-window-shell-tier="mac_native"');
    expect(markup).toContain("--window-shell-sidebar-width:420px");
  });

  test("applies native shell tier tokens in native mac mode", () => {
    const shellState = {
      chromeVariant: "mac",
      tier: "mac_native",
      toolbarHeight: 56,
      trafficLightInsetLeading: 78,
      sidebarHeaderHeight: 40,
      sidebarWidth: 420,
    } satisfies WindowShellState;

    const markup = renderToStaticMarkup(
      <MainWebviewApp initialWindowShellState={shellState} />,
    );

    expect(markup).toContain('data-window-shell-tier="mac_native"');
    expect(markup).toContain("--window-shell-sidebar-width:420px");
    expect(markup).toContain('data-main-content-visual-variant="native"');
    expect(markup).toContain('data-native-floating-controls="true"');
  });
});
