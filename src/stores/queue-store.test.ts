import { beforeEach, describe, expect, test } from "vitest";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";
import { createQueueStore, useQueueStore } from "./queue-store";

type QueueStoreWithDragReorder = ReturnType<typeof useQueueStore.getState> & {
  reorderBySongId?: (activeId: string, overId: string) => void;
};

interface FakeChannel {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
}

describe("queue-store drag reorder", () => {
  beforeEach(() => {
    useQueueStore.setState({ queue: [], isOpen: false });
  });

  test("moves the dragged song before the hovered song", () => {
    useQueueStore.setState({ queue: ["song-a", "song-b", "song-c"] });

    const store = useQueueStore.getState() as QueueStoreWithDragReorder;

    expect(typeof store.reorderBySongId).toBe("function");

    store.reorderBySongId?.("song-c", "song-a");

    expect(useQueueStore.getState().queue).toEqual([
      "song-c",
      "song-a",
      "song-b",
    ]);
  });

  test("moves the dragged song after the hovered song when dragging downward", () => {
    useQueueStore.setState({ queue: ["song-a", "song-b", "song-c"] });

    const store = useQueueStore.getState() as QueueStoreWithDragReorder;

    expect(typeof store.reorderBySongId).toBe("function");

    store.reorderBySongId?.("song-a", "song-c");

    expect(useQueueStore.getState().queue).toEqual([
      "song-b",
      "song-c",
      "song-a",
    ]);
  });

  test("ignores drag reorder when one of the songs is missing", () => {
    useQueueStore.setState({ queue: ["song-a", "song-b", "song-c"] });

    const store = useQueueStore.getState() as QueueStoreWithDragReorder;

    expect(typeof store.reorderBySongId).toBe("function");

    store.reorderBySongId?.("song-x", "song-a");

    expect(useQueueStore.getState().queue).toEqual([
      "song-a",
      "song-b",
      "song-c",
    ]);
  });

  test("syncs queue mutations across webview contexts", () => {
    const channelsByName = new Map<string, Set<FakeChannel>>();
    const channelFactory = (name: string) => {
      const peers = channelsByName.get(name) ?? new Set<FakeChannel>();
      channelsByName.set(name, peers);

      const channel: FakeChannel = {
        onmessage: null as ((event: { data: unknown }) => void) | null,
        postMessage(data: unknown) {
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

    const primary = createQueueStore(
      createWebviewSyncChannel("queue", {
        channelFactory,
        originId: "primary",
      }),
    );
    const secondary = createQueueStore(
      createWebviewSyncChannel("queue", {
        channelFactory,
        originId: "secondary",
      }),
    );

    primary.store.getState().addToQueue("song-a");
    primary.store.getState().addToQueue("song-b");

    expect(secondary.store.getState().queue).toEqual(["song-a", "song-b"]);

    primary.dispose();
    secondary.dispose();
  });
});
