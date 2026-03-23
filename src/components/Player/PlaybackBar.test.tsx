// @vitest-environment jsdom

import { renderToStaticMarkup } from "react-dom/server";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, test, vi, beforeEach } from "vitest";
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

let measuredWidth = 1280;
let resizeObserverCallback: ResizeObserverCallback | null = null;

beforeEach(() => {
  measuredWidth = 1280;
  resizeObserverCallback = null;
  (
    globalThis as typeof globalThis & {
      IS_REACT_ACT_ENVIRONMENT?: boolean;
    }
  ).IS_REACT_ACT_ENVIRONMENT = true;

  Object.defineProperty(HTMLElement.prototype, "getBoundingClientRect", {
    configurable: true,
    value: () => ({
      width: measuredWidth,
      height: 80,
      top: 0,
      right: measuredWidth,
      bottom: 80,
      left: 0,
      x: 0,
      y: 0,
      toJSON: () => ({}),
    }),
  });

  class MockResizeObserver implements ResizeObserver {
    observe() {}

    unobserve() {}

    disconnect() {}

    takeRecords() {
      return [];
    }

    constructor(callback: ResizeObserverCallback) {
      resizeObserverCallback = callback;
    }
  }

  vi.stubGlobal("ResizeObserver", MockResizeObserver);
});

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
    expect(markup).toContain('data-playback-zone="left"');
    expect(markup).toContain('data-playback-zone="center"');
    expect(markup).toContain('data-playback-zone="right"');
    expect(markup).toContain('data-now-playing-density="tight"');
    expect(markup).toContain('data-play-controls-density="tight"');
    expect(markup).toContain('data-seek-bar-density="tight"');
    expect(markup).toContain('data-volume-sliders-density="tight"');
    expect(markup).toContain("Queue button");
    expect(markup).toContain("audio-level-slider w-10");
  });

  test("keeps one shared structure while exposing the native playback bar variant", () => {
    const markup = renderToStaticMarkup(
      <PlaybackBar shellTier="mac_native" densityOverride="relaxed" />,
    );

    expect(markup).toContain('data-playback-bar-visual-variant="native"');
    expect(markup).toContain('data-playback-zone="left"');
    expect(markup).toContain('data-playback-zone="center"');
    expect(markup).toContain('data-playback-zone="right"');
  });

  test("measures the container width and updates the density through ResizeObserver", async () => {
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<PlaybackBar />);
    });

    expect(container.innerHTML).toContain(
      'data-playback-bar-density="relaxed"',
    );

    measuredWidth = 900;

    await act(async () => {
      resizeObserverCallback?.(
        [] as ResizeObserverEntry[],
        {} as ResizeObserver,
      );
    });

    expect(container.innerHTML).toContain('data-playback-bar-density="tight"');

    await act(async () => {
      root.unmount();
    });

    container.remove();
  });

  test("collapses the now playing zone at narrow widths instead of letting controls overlap", async () => {
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<PlaybackBar />);
    });

    measuredWidth = 720;

    await act(async () => {
      resizeObserverCallback?.(
        [] as ResizeObserverEntry[],
        {} as ResizeObserver,
      );
    });

    expect(container.innerHTML).not.toContain('data-playback-zone="left"');
    expect(container.innerHTML).not.toContain("Now playing");
    expect(container.innerHTML).toContain('data-playback-zone="center"');
    expect(container.innerHTML).toContain('data-playback-zone="right"');
    expect(container.innerHTML).toContain("Queue button");
    expect(container.innerHTML).toContain("audio-level-slider w-10");

    await act(async () => {
      root.unmount();
    });

    container.remove();
  });
});
