import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { Toolbar } from "./Toolbar";

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/components/Library/ImportButton", () => ({
  ImportButton: ({ children }: { children: React.ReactNode }) => children,
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
});
