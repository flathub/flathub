import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import type { WindowShellState } from "@/lib/window-shell";
import { SidebarWebviewApp } from "./SidebarWebviewApp";

vi.mock("@/lib/app-shortcuts", () => ({
  getShortcutPlatform: () => "mac",
}));

describe("SidebarWebviewApp", () => {
  test("defaults child sidebar hosts to native shell tokens before IPC hydration", () => {
    const markup = renderToStaticMarkup(<SidebarWebviewApp />);

    expect(markup).toContain('data-window-shell-tier="mac_native"');
    expect(markup).toContain('data-native-sidebar-header="true"');
  });

  test("renders native sidebar header controls above search", () => {
    const shellState = {
      chromeVariant: "mac",
      tier: "mac_native",
      toolbarHeight: 56,
      trafficLightInsetLeading: 78,
      sidebarWidth: 420,
    } satisfies WindowShellState;

    const markup = renderToStaticMarkup(
      <SidebarWebviewApp initialWindowShellState={shellState} />,
    );

    expect(markup).toContain('data-native-sidebar-header="true"');
    expect(markup).toContain('data-native-sidebar-header-layout="split"');
    expect(markup).toContain(
      "padding-inline-start:var(--window-shell-leading-controls-space)",
    );
    expect(markup).toContain('aria-label="Toggle Sidebar"');
    expect(markup).toContain('aria-label="Import"');
    expect(markup).toContain('data-sidebar-visual-variant="native"');
    expect(markup).toContain("lucide-plus");
    expect(markup).not.toContain("lucide-cloud-upload");
  });
});
