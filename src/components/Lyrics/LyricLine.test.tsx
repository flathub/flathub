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

  test("renders word-level states for the active line without changing lyric timing behavior", () => {
    const markup = renderToStaticMarkup(
      <LyricLine
        line={{
          time_ms: 1000,
          text: "alpha beta gamma",
          words: [
            { text: "alpha", time_ms: 1000 },
            { text: "beta", time_ms: 1500 },
            { text: "gamma", time_ms: 2000 },
          ],
        }}
        state="active"
        adjustedMs={1600}
      />,
    );

    expect(markup).toContain("text-[var(--color-text-dimmer)]");
    expect(markup).toContain("text-white");
    expect(markup).toContain("text-[var(--color-active)]");
  });
});
