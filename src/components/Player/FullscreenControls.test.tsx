import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { FullscreenControls } from "./FullscreenControls";

const { mockUseMouseIdle } = vi.hoisted(() => ({
  mockUseMouseIdle: vi.fn(() => true),
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("./PlayControls", () => ({
  PlayControls: () => <div>Play controls</div>,
}));

vi.mock("./SeekBar", () => ({
  SeekBar: () => <div>Seek bar</div>,
}));

vi.mock("@/hooks/use-mouse-idle", () => ({
  useMouseIdle: mockUseMouseIdle,
}));

vi.mock("@/lib/fullscreen-player", () => ({
  closeFullscreenPlayer: vi.fn(),
}));

describe("FullscreenControls", () => {
  test("keeps the close affordance interactive even while the cursor is idle", () => {
    mockUseMouseIdle.mockReturnValue(true);

    const markup = renderToStaticMarkup(<FullscreenControls />);

    expect(markup).not.toContain(
      "top-4 z-50 transition-opacity duration-300 pointer-events-none",
    );
    expect(markup).toContain("opacity-35");
  });
});
