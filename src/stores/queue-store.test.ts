import { beforeEach, describe, expect, test } from "vitest";
import { useQueueStore } from "./queue-store";

type QueueStoreWithDragReorder = ReturnType<typeof useQueueStore.getState> & {
  reorderBySongId?: (activeId: string, overId: string) => void;
};

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
});
