// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { beforeEach, describe, expect, test, vi } from "vitest";
import { FullscreenPlayerView } from "./FullscreenPlayerView";

const { mockAnnounceLocalAudienceOutputActive } = vi.hoisted(() => ({
  mockAnnounceLocalAudienceOutputActive: vi.fn(),
}));

vi.mock("@/components/Playback/PlaybackStage", () => ({
  PlaybackStage: () => <div>Stage</div>,
}));

vi.mock("./FullscreenControls", () => ({
  FullscreenControls: () => <div>Controls</div>,
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

vi.mock("@/lib/plain-text-page-controls", () => ({
  announceLocalAudienceOutputActive: mockAnnounceLocalAudienceOutputActive,
}));

describe("FullscreenPlayerView audience state events", () => {
  beforeEach(() => {
    mockAnnounceLocalAudienceOutputActive.mockReset();
    mockAnnounceLocalAudienceOutputActive.mockResolvedValue(undefined);
  });

  test("announces when the local audience window opens and closes", async () => {
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<FullscreenPlayerView />);
    });

    expect(mockAnnounceLocalAudienceOutputActive).toHaveBeenCalledWith(true);

    await act(async () => {
      root.unmount();
    });

    expect(mockAnnounceLocalAudienceOutputActive).toHaveBeenLastCalledWith(
      false,
    );
    container.remove();
  });
});
