import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { FullscreenPlayerView } from "./FullscreenPlayerView";

vi.mock("@/components/Playback/PlaybackStage", () => ({
  PlaybackStage: () => <div data-testid="playback-stage">Stage</div>,
}));

vi.mock("./FullscreenControls", () => ({
  FullscreenControls: () => (
    <div data-testid="fullscreen-controls">Controls</div>
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
  test("uses a flex fullscreen stage container so mirrored content can fill the window", () => {
    const markup = renderToStaticMarkup(<FullscreenPlayerView />);

    expect(markup).toContain(
      "relative flex h-screen w-screen flex-col bg-black",
    );
    expect(markup).toContain("flex flex-1 overflow-hidden");
    expect(markup).toContain("playback-stage");
  });
});
