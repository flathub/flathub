import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { SeekBar } from "./SeekBar";

const { mockPlayerState } = vi.hoisted(() => ({
  mockPlayerState: {
    snapshot: {
      duration_ms: 276000,
    },
    positionMs: 92000,
    seek: vi.fn(),
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
}));

describe("SeekBar", () => {
  test("keeps a minimum safe width for the whole control and the draggable rail", () => {
    const markup = renderToStaticMarkup(<SeekBar density="tight" />);

    expect(markup).toContain("min-w-[180px]");
    expect(markup).toContain("min-w-[120px]");
    expect(markup).toContain("w-[3.25rem]");
    expect(markup).toContain("font-[tabular-nums]");
    expect(markup).toContain("whitespace-nowrap");
  });
});
