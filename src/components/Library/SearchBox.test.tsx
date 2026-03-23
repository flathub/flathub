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
  test("renders the shared search field by default", () => {
    const markup = renderToStaticMarkup(<SearchBox />);

    expect(markup).toContain('data-search-visual-variant="default"');
  });

  test("renders the native search field variant hooks for mac native sidebars", () => {
    const markup = renderToStaticMarkup(<SearchBox variant="native" />);

    expect(markup).toContain('data-search-visual-variant="native"');
  });
});
