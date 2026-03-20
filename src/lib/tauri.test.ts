import { beforeEach, describe, expect, test, vi } from "vitest";

const { mockInvoke } = vi.hoisted(() => ({
  mockInvoke: vi.fn(),
}));

vi.mock("@tauri-apps/api/core", () => ({
  invoke: mockInvoke,
}));

import { getCdgFrame } from "./tauri";

describe("tauri API wrappers", () => {
  beforeEach(() => {
    mockInvoke.mockReset();
    mockInvoke.mockResolvedValue(null);
  });

  test("sends the backend positionMs payload name", async () => {
    await getCdgFrame(123.6);

    expect(mockInvoke).toHaveBeenCalledWith("get_cdg_frame", {
      positionMs: 124,
    });
  });

  test("syncs the native AirPlay route picker bounds", async () => {
    const { syncAirPlayRoutePicker } = await import("./tauri");

    await syncAirPlayRoutePicker({
      left: 12,
      top: 34,
      width: 140,
      height: 28,
    });

    expect(mockInvoke).toHaveBeenCalledWith("sync_airplay_route_picker", {
      bounds: {
        left: 12,
        top: 34,
        width: 140,
        height: 28,
      },
    });
  });

  test("syncs audience state to the AirPlay backend", async () => {
    const { syncAirPlayAudienceState } = await import("./tauri");

    await syncAirPlayAudienceState({
      mode: "lyrics",
      songId: "song-1",
      isPlaying: true,
      positionMs: 3210,
      lines: [{ time_ms: 3000, text: "Line", words: null }],
      activeLineIndex: 0,
      offsetMs: 100,
      lyricsFontStep: 1,
    });

    expect(mockInvoke).toHaveBeenCalledWith("sync_airplay_audience_state", {
      payload: {
        mode: "lyrics",
        songId: "song-1",
        isPlaying: true,
        positionMs: 3210,
        lines: [{ time_ms: 3000, text: "Line", words: null }],
        activeLineIndex: 0,
        offsetMs: 100,
        lyricsFontStep: 1,
      },
    });
  });
});
