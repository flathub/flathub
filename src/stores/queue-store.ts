import { create } from "zustand";
import {
  createWebviewSyncChannel,
  type WebviewSyncChannel,
} from "@/runtime/webview-sync";

const MAX_PLAY_HISTORY = 500;

interface QueueState {
  queue: string[];
  playHistory: string[];
  isOpen: boolean;

  addToQueue: (songId: string) => void;
  playNext: (songId: string) => void;
  removeFromQueue: (index: number) => void;
  removeSongIds: (songIds: string[]) => void;
  reorder: (fromIndex: number, toIndex: number) => void;
  reorderBySongId: (activeId: string, overId: string) => void;
  clearQueue: () => void;
  dequeue: () => string | undefined;
  pushToHistory: (songId: string) => void;
  popFromHistory: () => string | undefined;
  clearHistory: () => void;
  togglePanel: () => void;
}

interface QueueSyncSnapshot {
  queue: string[];
  playHistory: string[];
}

function reorderQueue(queue: string[], fromIndex: number, toIndex: number) {
  if (
    fromIndex === toIndex ||
    fromIndex < 0 ||
    toIndex < 0 ||
    fromIndex >= queue.length ||
    toIndex >= queue.length
  ) {
    return queue;
  }

  const nextQueue = [...queue];
  const [moved] = nextQueue.splice(fromIndex, 1);

  if (!moved) {
    return queue;
  }

  nextQueue.splice(toIndex, 0, moved);
  return nextQueue;
}

function queuesEqual(left: string[], right: string[]) {
  return (
    left.length === right.length &&
    left.every((songId, index) => songId === right[index])
  );
}

export function createQueueStore(
  syncChannel: WebviewSyncChannel<QueueSyncSnapshot> = createWebviewSyncChannel<QueueSyncSnapshot>(
    "openkara.queue",
  ),
) {
  const store = create<QueueState>((set, get) => {
    const syncQueue = (nextQueue: string[], nextHistory?: string[]) => {
      const history = nextHistory ?? get().playHistory;
      if (queuesEqual(get().queue, nextQueue)) {
        set({ playHistory: history });
        syncChannel.publish({ queue: nextQueue, playHistory: history });
        return;
      }

      set({ queue: nextQueue, playHistory: history });
      syncChannel.publish({ queue: nextQueue, playHistory: history });
    };

    return {
      queue: [],
      playHistory: [],
      isOpen: false,

      addToQueue: (songId) => {
        const { queue } = get();
        if (!queue.includes(songId)) {
          syncQueue([...queue, songId]);
        }
      },

      playNext: (songId) => {
        const { queue } = get();
        const filtered = queue.filter((id) => id !== songId);
        syncQueue([songId, ...filtered]);
      },

      removeFromQueue: (index) => {
        syncQueue(get().queue.filter((_, i) => i !== index));
      },

      removeSongIds: (songIds) => {
        const ids = new Set(songIds);
        syncQueue(get().queue.filter((songId) => !ids.has(songId)));
      },

      reorder: (fromIndex, toIndex) => {
        const queue = reorderQueue(get().queue, fromIndex, toIndex);
        if (!queuesEqual(queue, get().queue)) {
          syncQueue(queue);
        }
      },

      reorderBySongId: (activeId, overId) => {
        const currentQueue = get().queue;
        const fromIndex = currentQueue.indexOf(activeId);
        const toIndex = currentQueue.indexOf(overId);
        const queue = reorderQueue(currentQueue, fromIndex, toIndex);

        if (!queuesEqual(queue, currentQueue)) {
          syncQueue(queue);
        }
      },

      clearQueue: () => syncQueue([]),

      dequeue: () => {
        const { queue } = get();
        if (queue.length === 0) return undefined;
        const [next, ...rest] = queue;
        syncQueue(rest);
        return next;
      },

      pushToHistory: (songId) => {
        const { playHistory, queue } = get();
        const deduped = playHistory.filter((id) => id !== songId);
        const next = [...deduped, songId];
        // Cap the history stack to prevent unbounded growth.
        // 500 entries covers ~25 hours of continuous playback; far beyond
        // any real session yet small enough to be a non-issue for memory.
        const capped =
          next.length > MAX_PLAY_HISTORY
            ? next.slice(next.length - MAX_PLAY_HISTORY)
            : next;
        syncQueue(queue, capped);
      },

      popFromHistory: () => {
        const { playHistory, queue } = get();
        if (playHistory.length === 0) return undefined;
        syncQueue(queue, playHistory.slice(0, -1));
        return playHistory[playHistory.length - 1];
      },

      clearHistory: () => {
        const { queue } = get();
        syncQueue(queue, []);
      },

      togglePanel: () => set((state) => ({ isOpen: !state.isOpen })),
    };
  });

  const unsubscribe = syncChannel.subscribe(({ queue, playHistory }) => {
    store.setState({ queue, playHistory });
  });

  return {
    store,
    dispose() {
      unsubscribe();
      syncChannel.close();
    },
  };
}

const defaultQueueStore = createQueueStore();

export const useQueueStore = defaultQueueStore.store;
