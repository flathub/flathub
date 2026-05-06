import { describe, expect, test, vi } from "vitest";
import { createTrailingRateLimiter } from "./rate-limit";

describe("createTrailingRateLimiter", () => {
  test("runs the first call immediately and coalesces rapid updates", () => {
    vi.useFakeTimers();
    const calls: number[] = [];
    const limiter = createTrailingRateLimiter((value: number) => {
      calls.push(value);
    }, 33);

    limiter(1);
    limiter(2);
    limiter(3);

    expect(calls).toEqual([1]);

    vi.advanceTimersByTime(32);
    expect(calls).toEqual([1]);

    vi.advanceTimersByTime(1);
    expect(calls).toEqual([1, 3]);

    vi.useRealTimers();
  });

  test("flushes the latest pending value immediately", () => {
    vi.useFakeTimers();
    const calls: number[] = [];
    const limiter = createTrailingRateLimiter((value: number) => {
      calls.push(value);
    }, 33);

    limiter(10);
    limiter(20);
    limiter.flush();

    expect(calls).toEqual([10, 20]);

    vi.advanceTimersByTime(33);
    expect(calls).toEqual([10, 20]);

    vi.useRealTimers();
  });
});
