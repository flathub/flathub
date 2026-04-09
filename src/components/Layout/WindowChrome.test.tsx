import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import type { WindowShellState } from "@/lib/window-shell";
import { WindowChrome } from "./WindowChrome";

const macShellState = {
  chromeVariant: "mac",
  tier: "mac",
  toolbarHeight: 48,
  trafficLightInsetLeading: 78,
  sidebarHeaderHeight: 28,
  sidebarWidth: 260,
} satisfies WindowShellState;

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/components/Library/ImportButton", () => ({
  ImportButton: ({ children }: { children: React.ReactNode }) => children,
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({
    children,
    label,
    shortcut,
  }: {
    children: React.ReactNode;
    label: string;
    shortcut?: string;
  }) => (
    <span data-tooltip-label={label} data-tooltip-shortcut={shortcut}>
      {children}
    </span>
  ),
}));

vi.mock("@/components/Player/MonitorPicker", () => ({
  MonitorPicker: () => null,
}));

describe("WindowChrome", () => {
  test("renders the existing toolbar on macOS", () => {
    const markup = renderToStaticMarkup(
      <WindowChrome
        platform="mac"
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        shellState={macShellState}
        settingsOpen={false}
        sidebarVisible
      />,
    );

    expect(markup).toContain("data-tauri-drag-region");
    expect(markup).toContain('data-window-shell-tier="mac"');
    expect(markup).not.toContain('aria-label="windowChrome.minimize"');
    expect(markup).not.toContain("windowChrome.file");
  });

  test("renders the desktop titlebar on Windows and Linux", () => {
    const markup = renderToStaticMarkup(
      <WindowChrome
        platform="windows"
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        settingsOpen={false}
        sidebarVisible
      />,
    );

    expect(markup).toContain("windowChrome.file");
    expect(markup).toContain("windowChrome.edit");
    expect(markup).toContain("windowChrome.window");
    expect(markup).toContain("windowChrome.help");
    expect(markup).toContain('aria-label="windowChrome.minimize"');
    expect(markup).toContain('aria-label="windowChrome.maximize"');
    expect(markup).toContain('aria-label="windowChrome.close"');
    expect(markup).not.toContain("data-tauri-drag-region");
  });
});
