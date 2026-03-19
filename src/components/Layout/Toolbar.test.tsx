import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { APP_SHORTCUTS, getShortcutDisplay } from "@/lib/app-shortcuts";
import { Toolbar } from "./Toolbar";

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

describe("Toolbar drag region", () => {
  test("keeps the toolbar root interactive and isolates drag affordances", () => {
    const markup = renderToStaticMarkup(
      <Toolbar
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        settingsOpen={false}
        sidebarVisible={false}
      />,
    );

    expect(markup).not.toContain(
      'bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)] px-4 shadow-[0_1px_0_rgba(255,255,255,0.02)] backdrop-blur-xl" data-tauri-drag-region',
    );
    expect(markup).toContain("data-tauri-drag-region");
  });

  test("does not insert a sidebar-hidden spacer before the toggle button", () => {
    const markup = renderToStaticMarkup(
      <Toolbar
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        settingsOpen={false}
        sidebarVisible={false}
      />,
    );

    expect(markup).not.toContain("w-[54px]");
  });

  test("shows import tooltip metadata with the shared shortcut", () => {
    const markup = renderToStaticMarkup(
      <Toolbar
        onToggleSidebar={() => {}}
        onToggleSettings={() => {}}
        settingsOpen={false}
        sidebarVisible={true}
      />,
    );

    expect(markup).toContain('data-tooltip-label="toolbar.import"');
    expect(markup).toContain(
      `data-tooltip-shortcut="${getShortcutDisplay(APP_SHORTCUTS.importFiles)}"`,
    );
  });
});
