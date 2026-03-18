import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { FullscreenPlayerView } from "./FullscreenPlayerView";

vi.mock("@/components/Playback/PlaybackStage", () => ({
  PlaybackStage: ({
    presentation,
    bottomInsetPx,
  }: {
    presentation?: string;
    bottomInsetPx?: number;
  }) => (
    <div
      data-testid="playback-stage"
      data-presentation={presentation}
      data-bottom-inset={bottomInsetPx}
    >
      Stage
    </div>
  ),
}));

vi.mock("./FullscreenControls", () => ({
  FullscreenControls: ({
    onHeightChange,
  }: {
    onHeightChange?: (height: number) => void;
  }) => (
    <div
      data-testid="fullscreen-controls"
      data-has-height-change={String(typeof onHeightChange === "function")}
    >
      Controls
    </div>
  ),
}));

vi.mock("@/hooks/use-cdg-frame-receiver", () => ({
  useCdgFrameReceiver: () => {},
}));

vi.mock("@/hooks/use-lyrics-sync", () => ({
  useLyricsSync: () => {},
}));

vi.mock("@/hooks/use-playback-runtime", () => ({
  useFullscreenPlaybackRuntime: () => {},
  useLyricsAutoFetch: () => {},
}));

describe("FullscreenPlayerView", () => {
  test("passes audience presentation and a persistent bottom inset to the stage", () => {
    const markup = renderToStaticMarkup(<FullscreenPlayerView />);

    expect(markup).toContain(
      "relative flex h-screen w-screen flex-col bg-black",
    );
    expect(markup).toContain("flex flex-1 overflow-hidden");
    expect(markup).toContain('data-presentation="audience"');
    expect(markup).toContain('data-bottom-inset="144"');
    expect(markup).toContain('data-has-height-change="true"');
    expect(markup).toContain("playback-stage");
  });
});
