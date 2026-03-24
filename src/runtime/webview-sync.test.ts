import { describe, expect, test } from "vitest";
import { createWebviewSyncChannel } from "./webview-sync";

interface FakeMessageEvent {
  data: unknown;
}

interface FakeBroadcastChannelLike {
  onmessage: ((event: FakeMessageEvent) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
}

function createFakeBroadcastChannelFactory() {
  const channelsByName = new Map<string, Set<FakeBroadcastChannelLike>>();

  return (name: string): FakeBroadcastChannelLike => {
    const peers =
      channelsByName.get(name) ?? new Set<FakeBroadcastChannelLike>();
    channelsByName.set(name, peers);

    const channel: FakeBroadcastChannelLike = {
      onmessage: null,
      postMessage(data) {
        for (const peer of peers) {
          if (peer === channel) {
            continue;
          }

          peer.onmessage?.({ data });
        }
      },
      close() {
        peers.delete(channel);
      },
    };

    peers.add(channel);
    return channel;
  };
}

describe("webview sync channel", () => {
  test("delivers messages to other webview contexts on the same channel", () => {
    const channelFactory = createFakeBroadcastChannelFactory();
    const sender = createWebviewSyncChannel<{ queue: string[] }>("queue", {
      channelFactory,
      originId: "sender",
    });
    const receiver = createWebviewSyncChannel<{ queue: string[] }>("queue", {
      channelFactory,
      originId: "receiver",
    });

    let received: { queue: string[] } | null = null;
    receiver.subscribe((payload) => {
      received = payload;
    });

    sender.publish({ queue: ["song-a"] });

    expect(received).toEqual({ queue: ["song-a"] });

    sender.close();
    receiver.close();
  });

  test("does not echo a published message back into the same origin", () => {
    const channelFactory = createFakeBroadcastChannelFactory();
    const sender = createWebviewSyncChannel<{ queue: string[] }>("queue", {
      channelFactory,
      originId: "same-origin",
    });

    let callCount = 0;
    sender.subscribe(() => {
      callCount += 1;
    });

    sender.publish({ queue: ["song-a"] });

    expect(callCount).toBe(0);

    sender.close();
  });
});
