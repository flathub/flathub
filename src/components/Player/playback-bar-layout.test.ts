import { describe, expect, test } from "vitest";
import { getPlaybackBarDensity } from "./playback-bar-layout";

describe("getPlaybackBarDensity", () => {
  test("uses the relaxed density at and above 1120px", () => {
    expect(getPlaybackBarDensity(1120)).toBe("relaxed");
    expect(getPlaybackBarDensity(1320)).toBe("relaxed");
  });

  test("uses the compact density between 960px and 1119px", () => {
    expect(getPlaybackBarDensity(1119)).toBe("compact");
    expect(getPlaybackBarDensity(960)).toBe("compact");
  });

  test("uses the tight density below 960px", () => {
    expect(getPlaybackBarDensity(959)).toBe("tight");
    expect(getPlaybackBarDensity(760)).toBe("tight");
  });
});
