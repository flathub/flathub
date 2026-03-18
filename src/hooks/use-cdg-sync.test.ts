import { beforeEach, describe, expect, test, vi } from "vitest";
import {
  createLatestOnlyFrameForwarder,
  startCdgPollingLoop,
} from "./use-cdg-sync";

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

  test("drops stale fullscreen frame emissions while a previous send is in flight", async () => {
    let firstSendPending = true;
    let resolveFirstSend: () => void = () => {};
    const sent: string[] = [];
    const forward = createLatestOnlyFrameForwarder((payload) => {
      sent.push(payload);
      return new Promise<void>((resolve) => {
        if (firstSendPending) {
          firstSendPending = false;
          resolveFirstSend = resolve;
        } else {
          resolve();
        }
      });
    });

    forward("frame-1");
    forward("frame-2");
    forward("frame-3");

    expect(sent).toEqual(["frame-1"]);

    resolveFirstSend();
    await Promise.resolve();
    await Promise.resolve();

    expect(sent).toEqual(["frame-1", "frame-3"]);
  });
});
