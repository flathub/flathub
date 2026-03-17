import { create } from "zustand";

interface QueueState {
  queue: string[];
  isOpen: boolean;

  addToQueue: (songId: string) => void;
  playNext: (songId: string) => void;
  removeFromQueue: (index: number) => void;
  removeSongIds: (songIds: string[]) => void;
  reorder: (fromIndex: number, toIndex: number) => void;
  reorderBySongId: (activeId: string, overId: string) => void;
  clearQueue: () => void;
  dequeue: () => string | undefined;
  togglePanel: () => void;
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

export const useQueueStore = create<QueueState>((set, get) => ({
  queue: [],
  isOpen: false,

  addToQueue: (songId) => {
    const { queue } = get();
    if (!queue.includes(songId)) {
      set({ queue: [...queue, songId] });
    }
  },

  playNext: (songId) => {
    const { queue } = get();
    // Remove if already in queue, then insert at front
    const filtered = queue.filter((id) => id !== songId);
    set({ queue: [songId, ...filtered] });
  },

  removeFromQueue: (index) => {
    set((state) => ({
      queue: state.queue.filter((_, i) => i !== index),
    }));
  },

  removeSongIds: (songIds) => {
    const ids = new Set(songIds);
    set((state) => ({
      queue: state.queue.filter((songId) => !ids.has(songId)),
    }));
  },

  reorder: (fromIndex, toIndex) => {
    set((state) => {
      const queue = reorderQueue(state.queue, fromIndex, toIndex);

      if (queue === state.queue) {
        return state;
      }

      return { queue };
    });
  },

  reorderBySongId: (activeId, overId) => {
    set((state) => {
      const fromIndex = state.queue.indexOf(activeId);
      const toIndex = state.queue.indexOf(overId);
      const queue = reorderQueue(state.queue, fromIndex, toIndex);

      if (queue === state.queue) {
        return state;
      }

      return { queue };
    });
  },

  clearQueue: () => set({ queue: [] }),

  dequeue: () => {
    const { queue } = get();
    if (queue.length === 0) return undefined;
    const [next, ...rest] = queue;
    set({ queue: rest });
    return next;
  },

  togglePanel: () => set((state) => ({ isOpen: !state.isOpen })),
}));
