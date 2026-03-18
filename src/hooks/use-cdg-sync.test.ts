import { beforeEach, describe, expect, test, vi } from "vitest";
import { startCdgPollingLoop } from "./use-cdg-sync";

describe("startCdgPollingLoop", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });

  test("polls with a fixed interval so fullscreen updates do not depend on rAF", () => {
    const tick = vi.fn();

    const stop = startCdgPollingLoop(tick, {
      setInterval,
      clearInterval,
    });

    vi.advanceTimersByTime(100);

    expect(tick).toHaveBeenCalled();

    stop();
  });
});
