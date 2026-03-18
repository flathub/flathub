import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { FullscreenControls } from "./FullscreenControls";

const { mockUseMouseIdle } = vi.hoisted(() => ({
  mockUseMouseIdle: vi.fn(() => true),
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
  test("hides playback bar when cursor is idle", () => {
    mockUseMouseIdle.mockReturnValue(true);

    const markup = renderToStaticMarkup(<FullscreenControls />);

    expect(markup).toContain("pointer-events-none");
    expect(markup).toContain("opacity-0");
  });

  test("shows playback bar when cursor is active", () => {
    mockUseMouseIdle.mockReturnValue(false);

    const markup = renderToStaticMarkup(<FullscreenControls />);

    expect(markup).not.toContain("pointer-events-none");
    expect(markup).toContain("opacity-100");
  });
});
