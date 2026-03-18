import { beforeEach, describe, expect, test, vi } from "vitest";

describe("createCoalescingPainter", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.unstubAllGlobals();
    vi.doUnmock("@/lib/tauri");
  });

  test("loads without importing the tauri display-frame polling API", async () => {
    vi.resetModules();
    vi.doMock("@/lib/tauri", () => {
      throw new Error("use-cdg-frame-receiver must stay message-driven");
    });

    const module = await import("./use-cdg-frame-receiver");

    expect(typeof module.createCoalescingPainter).toBe("function");
  });

  test("coalesces multiple frames into the latest macrotask paint without requestAnimationFrame", async () => {
    vi.resetModules();
    vi.doUnmock("@/lib/tauri");
    const { createCoalescingPainter } =
      await import("./use-cdg-frame-receiver");
    const paint = vi.fn();
    const requestAnimationFrameSpy = vi.fn();
    vi.stubGlobal("requestAnimationFrame", requestAnimationFrameSpy);

    const painter = createCoalescingPainter<string>(paint);

    painter.enqueue("frame-1");
    painter.enqueue("frame-2");

    expect(paint).not.toHaveBeenCalled();
    expect(requestAnimationFrameSpy).not.toHaveBeenCalled();

    vi.runAllTimers();

    expect(paint).toHaveBeenCalledTimes(1);
    expect(paint).toHaveBeenCalledWith("frame-2");
  });
});
