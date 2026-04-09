import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { SearchBox } from "./SearchBox";

const { mockLibraryState } = vi.hoisted(() => ({
  mockLibraryState: {
    searchQuery: "",
    setSearchQuery: vi.fn(),
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/stores/library-store", () => ({
  useLibraryStore: Object.assign(
    (selector: (state: typeof mockLibraryState) => unknown) =>
      selector(mockLibraryState),
    {
      setState: vi.fn(),
    },
  ),
}));

describe("SearchBox", () => {
  test("renders the unified search field chrome", () => {
    const markup = renderToStaticMarkup(<SearchBox />);

    expect(markup).toContain('data-search-visual-variant="unified"');
    expect(markup).toContain("bg-[var(--sidebar-control-bg)]");
    expect(markup).toContain("border-[var(--sidebar-control-border)]");
    expect(markup).toContain("focus-within:bg-[var(--sidebar-row-overlay-bg)]");
    expect(markup).not.toContain("border-[var(--sidebar-row-selected-border)]");
  });
});
