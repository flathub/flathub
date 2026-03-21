import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { selectSyncDisplayPositionMs, usePlayerStore } from "./player-store";

describe("selectSyncDisplayPositionMs", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    usePlayerStore.setState({
      airPlayPlainTextPagePending: false,
      airPlayPlainTextPagePendingDirection: null,
    });
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  test("prefers the AirPlay displayed position when active", () => {
    expect(
      selectSyncDisplayPositionMs({
        positionMs: 1000,
        airPlayOutput: {
          active: true,
          audioActive: true,
          routeName: "Living Room TV",
          mode: "lyrics",
          phase: "playing",
          detail: null,
          displayedPositionMs: 1250,
          streamGeneration: 7,
          latencyMs: 200,
        },
      }),
    ).toBe(1250);
  });

  test("falls back to the local playback position when AirPlay is inactive", () => {
    expect(
      selectSyncDisplayPositionMs({
        positionMs: 1000,
        airPlayOutput: {
          active: false,
          audioActive: false,
          routeName: null,
          mode: "idle",
          phase: "idle",
          detail: null,
          displayedPositionMs: 1250,
          streamGeneration: 0,
          latencyMs: null,
        },
      }),
    ).toBe(1000);
  });

  test("clears AirPlay plain-text page feedback after the lock window elapses", () => {
    usePlayerStore.getState().startAirPlayPlainTextPagePending("prev", 900);

    expect(usePlayerStore.getState().airPlayPlainTextPagePending).toBe(true);
    expect(usePlayerStore.getState().airPlayPlainTextPagePendingDirection).toBe(
      "prev",
    );

    vi.advanceTimersByTime(900);

    expect(usePlayerStore.getState().airPlayPlainTextPagePending).toBe(false);
    expect(usePlayerStore.getState().airPlayPlainTextPagePendingDirection).toBe(
      null,
    );
  });
});
