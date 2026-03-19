import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { PlaybackBar } from "./PlaybackBar";

const { mockPlayerState } = vi.hoisted(() => ({
  mockPlayerState: {
    snapshot: {
      volume: 0.72,
    },
    setVolume: vi.fn(),
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) =>
      (
        ({
          "player.volume": "Volume",
          "player.mute": "Mute",
          "player.unmute": "Unmute",
        }) as const
      )[key] ?? key,
  }),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({
    children,
    label,
  }: {
    children: React.ReactNode;
    label: string;
  }) => <span data-tooltip-label={label}>{children}</span>,
}));

vi.mock("./NowPlayingInfo", () => ({
  NowPlayingInfo: ({ density }: { density?: string }) => (
    <div data-now-playing-density={density}>Now playing</div>
  ),
}));

vi.mock("./PlayControls", () => ({
  PlayControls: ({ density }: { density?: string }) => (
    <div data-play-controls-density={density}>Play controls</div>
  ),
}));

vi.mock("./SeekBar", () => ({
  SeekBar: ({ density }: { density?: string }) => (
    <div data-seek-bar-density={density}>Seek bar</div>
  ),
}));

vi.mock("./VolumeSliders", () => ({
  VolumeSliders: ({ density }: { density?: string }) => (
    <div data-volume-sliders-density={density}>Stem sliders</div>
  ),
}));

vi.mock("./QueueButton", () => ({
  QueueButton: () => <div>Queue button</div>,
}));

describe("PlaybackBar", () => {
  test("uses the shared audio level slider for master volume tooltip text", () => {
    const markup = renderToStaticMarkup(<PlaybackBar />);

    expect(markup).toContain('data-tooltip-label="Volume 72%"');
    expect(markup).toContain("audio-level-slider");
    expect(markup).not.toContain("title=");
  });

  test("forwards the tight density to the responsive children", () => {
    const markup = renderToStaticMarkup(
      <PlaybackBar densityOverride="tight" />,
    );

    expect(markup).toContain('data-playback-bar-density="tight"');
    expect(markup).toContain('data-now-playing-density="tight"');
    expect(markup).toContain('data-play-controls-density="tight"');
    expect(markup).toContain('data-seek-bar-density="tight"');
    expect(markup).toContain('data-volume-sliders-density="tight"');
    expect(markup).toContain("audio-level-slider w-12");
  });
});
