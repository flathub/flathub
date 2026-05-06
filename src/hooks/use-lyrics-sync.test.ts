import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { startLyricsSyncLoop, syncLyricsToPlayback } from "./use-lyrics-sync";

// Replace store modules with minimal stubs. selectCurrentPositionMs is a
// pure helper that extrapolates from the last synced position; the mock
// returns positionMs directly for deterministic test behaviour.
vi.mock("@/stores/player-store", () => ({
  usePlayerStore: { getState: vi.fn() },
  selectCurrentPositionMs: (state: { positionMs: number }) => state.positionMs,
  selectSyncDisplayPositionMs: (state: { positionMs: number }) =>
    state.positionMs,
}));

vi.mock("@/stores/lyrics-store", () => ({
  useLyricsStore: { getState: vi.fn() },
}));

// Four lyric lines spanning 0–3 s, one per second.
const SAMPLE_LINES = [
  { time_ms: 0, text: "Intro" },
  { time_ms: 1000, text: "Line one" },
  { time_ms: 2000, text: "Line two" },
  { time_ms: 3000, text: "Line three" },
];

// ---------------------------------------------------------------------------
// startLyricsSyncLoop – interval plumbing
// ---------------------------------------------------------------------------

describe("startLyricsSyncLoop", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  test("syncs lyrics on a fixed interval instead of relying on requestAnimationFrame", () => {
    const tick = vi.fn();

    const stop = startLyricsSyncLoop(tick, {
      setInterval,
      clearInterval,
    });

    vi.advanceTimersByTime(100);

    expect(tick).toHaveBeenCalled();

    stop();
  });
});

// ---------------------------------------------------------------------------
// syncLyricsToPlayback active-line behaviour
// ---------------------------------------------------------------------------

describe("syncLyricsToPlayback active-line behaviour", () => {
  afterEach(() => {
    vi.clearAllMocks();
  });

  /** Wire up the lyrics-store mock and return a spy for setActiveLineIndex. */
  function setupLyricsMock(lines = SAMPLE_LINES) {
    const setActiveLineIndex = vi.fn();
    vi.mocked(useLyricsStore.getState).mockReturnValue(
      // Partial stub – only the fields read by syncLyricsToPlayback.
      { lines, offsetMs: 0, setActiveLineIndex } as unknown as ReturnType<
        (typeof useLyricsStore)["getState"]
      >,
    );
    return setActiveLineIndex;
  }

  /** Wire up the player-store mock with the given playback state. */
  function setupPlayerMock(
    songId: string | null,
    isPlaying: boolean,
    positionMs: number,
  ) {
    vi.mocked(usePlayerStore.getState).mockReturnValue(
      // Partial stub – only the fields read by syncLyricsToPlayback and the
      // selectSyncDisplayPositionMs proxy (which reads positionMs).
      {
        snapshot: songId
          ? { song_id: songId, is_playing: isPlaying, position_ms: positionMs }
          : null,
        positionMs,
      } as unknown as ReturnType<(typeof usePlayerStore)["getState"]>,
    );
  }

  test("updates the active line while playing", () => {
    const setActiveLineIndex = setupLyricsMock();
    // positionMs=2500 ms falls after the 2000 ms line → index 2
    setupPlayerMock("song-1", true, 2500);

    const ref = { current: -1 };
    syncLyricsToPlayback(ref);

    expect(setActiveLineIndex).toHaveBeenCalledWith(2);
    expect(ref.current).toBe(2);
  });

  test("updates the active line when paused after a seek (seek-while-paused fix)", () => {
    const setActiveLineIndex = setupLyricsMock();
    // Paused but a song is loaded; the user seeked to 3000 ms.
    // The guard now checks song_id instead of is_playing so the highlight
    // still updates immediately — this is the core of the bug fix.
    setupPlayerMock("song-1", false, 3000);

    const ref = { current: -1 };
    syncLyricsToPlayback(ref);

    // positionMs=3000 hits the last line at 3000 ms → index 3
    expect(setActiveLineIndex).toHaveBeenCalledWith(3);
    expect(ref.current).toBe(3);
  });

  test("does not update the active line when no song is loaded", () => {
    const setActiveLineIndex = setupLyricsMock();
    // No snapshot ⇒ song_id is absent; the function must return early.
    setupPlayerMock(null, false, 3000);

    const ref = { current: -1 };
    syncLyricsToPlayback(ref);

    expect(setActiveLineIndex).not.toHaveBeenCalled();
    expect(ref.current).toBe(-1);
  });

  test("does not call setActiveLineIndex when the computed index has not changed", () => {
    const setActiveLineIndex = setupLyricsMock();
    // ref already at index 2; position still maps to index 2 → no-op.
    setupPlayerMock("song-1", true, 2500);

    const ref = { current: 2 };
    syncLyricsToPlayback(ref);

    expect(setActiveLineIndex).not.toHaveBeenCalled();
  });

  test("does not update when the lines array is empty", () => {
    const setActiveLineIndex = setupLyricsMock([]);
    setupPlayerMock("song-1", true, 2500);

    const ref = { current: -1 };
    syncLyricsToPlayback(ref);

    expect(setActiveLineIndex).not.toHaveBeenCalled();
  });
});
