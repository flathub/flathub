import { describe, expect, test, vi } from "vitest";
import {
  startCdgSyncReceiver,
  startCdgSyncRequestListener,
  type CdgSyncChannel,
} from "./cdg-sync-channel";

function createFakeChannel(): CdgSyncChannel {
  const listeners = new Set<(event: { data: unknown }) => void>();

  return {
    addEventListener: vi.fn((type, listener) => {
      if (type === "message") {
        listeners.add(listener as (event: { data: unknown }) => void);
      }
    }),
    removeEventListener: vi.fn((type, listener) => {
      if (type === "message") {
        listeners.delete(listener as (event: { data: unknown }) => void);
      }
    }),
    close: vi.fn(),
    postMessage(message) {
      for (const listener of listeners) {
        listener({ data: message });
      }
    },
  };
}

describe("cdg-sync-channel", () => {
  test("registers broadcast listeners before requesting sync", () => {
    const channel = createFakeChannel();
    const callOrder: string[] = [];

    const recordingChannel: CdgSyncChannel = {
      ...channel,
      addEventListener: vi.fn((type, listener) => {
        callOrder.push(`listen:${type}`);
        channel.addEventListener(type, listener);
      }),
      removeEventListener: channel.removeEventListener,
      close: channel.close,
      postMessage: (message) => {
        callOrder.push(`post:${message.type}`);
        channel.postMessage(message);
      },
    };

    startCdgSyncReceiver({
      channel: recordingChannel,
      onFrame: vi.fn(),
      onClear: vi.fn(),
      onStatus: vi.fn(),
    });

    expect(callOrder).toEqual(["listen:message", "post:request-sync"]);
  });

  test("replays the latest status and frame when sync is requested", () => {
    const channel = createFakeChannel();
    const onStatus = vi.fn();
    const onFrame = vi.fn();

    startCdgSyncRequestListener({
      channel,
      getSnapshot: () => ({
        status: { songId: "song-1", hasCdg: true },
        frame: new Uint8Array([1, 2, 3]).buffer,
      }),
    });
    startCdgSyncReceiver({
      channel,
      onFrame,
      onClear: vi.fn(),
      onStatus,
    });

    expect(onStatus).toHaveBeenCalledWith({ songId: "song-1", hasCdg: true });
    expect(onFrame).toHaveBeenCalledOnce();
    expect(new Uint8Array(onFrame.mock.calls[0][0])).toEqual(
      new Uint8Array([1, 2, 3]),
    );
  });
});
