import { create } from "zustand";

interface QueueState {
  queue: string[];
  isOpen: boolean;

  addToQueue: (songId: string) => void;
  playNext: (songId: string) => void;
  removeFromQueue: (index: number) => void;
  reorder: (fromIndex: number, toIndex: number) => void;
  clearQueue: () => void;
  dequeue: () => string | undefined;
  togglePanel: () => void;
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

  reorder: (fromIndex, toIndex) => {
    set((state) => {
      const newQueue = [...state.queue];
      const [moved] = newQueue.splice(fromIndex, 1);
      newQueue.splice(toIndex, 0, moved);
      return { queue: newQueue };
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
