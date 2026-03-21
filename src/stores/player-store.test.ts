import { describe, expect, test } from "vitest";
import { selectSyncDisplayPositionMs } from "./player-store";

describe("selectSyncDisplayPositionMs", () => {
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
});
