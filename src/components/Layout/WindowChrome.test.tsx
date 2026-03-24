import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { WindowChrome } from "./WindowChrome";

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
  test("renders nothing on macOS (chrome lives in AppLayout + Sidebar)", () => {
    const markup = renderToStaticMarkup(
      <WindowChrome
        platform="mac"
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        settingsOpen={false}
        sidebarVisible
      />,
    );

    expect(markup).toBe("");
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
