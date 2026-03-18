import { beforeEach, describe, expect, test, vi } from "vitest";
import { startLyricsSyncLoop } from "./use-lyrics-sync";

describe("startLyricsSyncLoop", () => {
  beforeEach(() => {
    vi.useFakeTimers();
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
