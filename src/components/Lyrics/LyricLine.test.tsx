import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { LyricLine } from "./LyricLine";

const { mockSeek } = vi.hoisted(() => ({
  mockSeek: vi.fn(),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: { seek: typeof mockSeek }) => unknown) =>
    selector({ seek: mockSeek }),
}));

describe("LyricLine", () => {
  test("does not render plain-text lyrics as clickable", () => {
    const markup = renderToStaticMarkup(
      <LyricLine
        line={{ time_ms: 0, text: "plain line", words: null }}
        state="plain"
        adjustedMs={0}
      />,
    );

    expect(markup).not.toContain("cursor-pointer");
  });
});
