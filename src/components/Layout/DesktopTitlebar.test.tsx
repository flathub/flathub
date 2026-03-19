import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { DesktopTitlebar } from "./DesktopTitlebar";

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

describe("DesktopTitlebar", () => {
  test("shows the restore affordance when the window is maximized", () => {
    const markup = renderToStaticMarkup(
      <DesktopTitlebar
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        settingsOpen={false}
        sidebarVisible
        initialIsMaximized
      />,
    );

    expect(markup).toContain('data-maximized="true"');
    expect(markup).toContain('aria-label="windowChrome.restore"');
    expect(markup).not.toContain('aria-label="windowChrome.maximize"');
  });
});
