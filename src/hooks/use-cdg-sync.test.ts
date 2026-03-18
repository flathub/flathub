import { describe, expect, test, vi } from "vitest";
import { startCdgPositionSync } from "./use-cdg-sync";

describe("startCdgPositionSync", () => {
  test("ticks only when playback crosses a new CDG sync bucket", () => {
    const tick = vi.fn();
    let listener:
      | ((positionMs: number, previousPositionMs: number) => void)
      | null = null;

    const emitPosition = (positionMs: number, previousPositionMs: number) => {
      expect(listener).not.toBeNull();
      listener!(positionMs, previousPositionMs);
    };

    const stop = startCdgPositionSync(tick, (nextListener) => {
      listener = nextListener;
      return () => {
        listener = null;
      };
    });

    emitPosition(10, 0);
    emitPosition(20, 10);
    emitPosition(34, 20);
    emitPosition(40, 34);
    emitPosition(67, 40);

    expect(tick).toHaveBeenCalledTimes(2);

    stop();
    expect(listener).toBeNull();
  });
});
