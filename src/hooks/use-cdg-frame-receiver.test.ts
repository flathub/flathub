import { describe, expect, test, vi } from "vitest";
import { startCdgFrameReceiver } from "./use-cdg-frame-receiver";

describe("startCdgFrameReceiver", () => {
  test("registers listeners before requesting a sync frame", async () => {
    const callOrder: string[] = [];

    const listen = vi.fn(async (event: string) => {
      callOrder.push(`listen:${event}`);
      return () => {};
    });
    const emitSyncRequest = vi.fn(async () => {
      callOrder.push("emit:cdg-request-sync");
    });

    await startCdgFrameReceiver({
      listen,
      emitSyncRequest,
      onFrame: vi.fn(),
      onClear: vi.fn(),
      onStatus: vi.fn(),
    });

    expect(callOrder).toEqual([
      "listen:cdg-frame",
      "listen:cdg-clear",
      "listen:cdg-status",
      "emit:cdg-request-sync",
    ]);
  });
});
