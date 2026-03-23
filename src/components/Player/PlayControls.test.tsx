import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { PlayControls } from "./PlayControls";

const { mockPlayerState } = vi.hoisted(() => ({
  mockPlayerState: {
    snapshot: {
      song_id: "song-1",
      is_playing: false,
    },
    resume: vi.fn(),
    pause: vi.fn(),
    skipBack: vi.fn(),
    skipForward: vi.fn(),
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

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({ children }: { children: React.ReactNode }) => children,
}));

describe("PlayControls", () => {
  test("exposes the native visual variant while keeping the shared control cluster", () => {
    const markup = renderToStaticMarkup(
      <PlayControls shellTier="mac_native" />,
    );

    expect(markup).toContain('data-play-controls-visual-variant="native"');
    expect(markup).toContain('aria-label="player.play"');
  });
});
