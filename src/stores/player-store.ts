import { create } from "zustand";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import { useLibraryStore } from "@/stores/library-store";
import type { PlaybackStateSnapshot, StemName } from "@/types/ipc";

interface PlayerState {
  snapshot: PlaybackStateSnapshot | null;
  positionMs: number;

  playSong: (songId: string) => Promise<void>;
  pause: () => Promise<void>;
  seek: (ms: number) => Promise<void>;
  setVolume: (level: number) => Promise<void>;
  setStemVolume: (stem: StemName, level: number) => Promise<void>;
  loadStems: () => Promise<void>;
  updatePosition: (ms: number) => void;
  updateSnapshot: (snapshot: PlaybackStateSnapshot) => void;
  loadState: () => Promise<void>;
}

export const usePlayerStore = create<PlayerState>((set) => ({
  snapshot: null,
  positionMs: 0,

  playSong: async (songId) => {
    try {
      const snapshot = await api.play(songId);
      set({ snapshot, positionMs: snapshot.position_ms });

      // Auto-load stems if separation was previously completed
      const sepStatus = useLibraryStore.getState().separationStatuses[songId];
      if (sepStatus?.state === "completed" && !snapshot.has_stems) {
        try {
          const updated = await api.loadStems();
          set({ snapshot: updated });
        } catch {
          // Stems loading failed silently — user can still play original audio
        }
      }
    } catch (e) {
      notifyError(e, () => usePlayerStore.getState().playSong(songId));
    }
  },

  pause: async () => {
    try {
      const snapshot = await api.pause();
      set({ snapshot, positionMs: snapshot.position_ms });
    } catch (e) {
      notifyError(e);
    }
  },

  seek: async (ms) => {
    const current = usePlayerStore.getState().snapshot;
    if (!current?.song_id) return;
    try {
      const clamped = Math.max(0, ms);
      const snapshot = await api.seek(clamped);
      set({ snapshot, positionMs: snapshot.position_ms });
    } catch (e) {
      notifyError(e);
    }
  },

  setVolume: async (level) => {
    try {
      const clamped = Math.max(0, Math.min(1, level));
      const snapshot = await api.setVolume(clamped);
      set({ snapshot });
    } catch (e) {
      notifyError(e);
    }
  },

  setStemVolume: async (stem, level) => {
    try {
      const clamped = Math.max(0, Math.min(1, level));
      const snapshot = await api.setStemVolume(stem, clamped);
      set({ snapshot });
    } catch (e) {
      notifyError(e);
    }
  },

  loadStems: async () => {
    try {
      const snapshot = await api.loadStems();
      set({ snapshot });
    } catch (e) {
      notifyError(e, () => usePlayerStore.getState().loadStems());
    }
  },

  updatePosition: (ms) => {
    // Ignore position events when paused — the pause snapshot already set
    // the correct position, and stale emitter events could reset it to 0.
    const { snapshot } = usePlayerStore.getState();
    if (snapshot && !snapshot.is_playing) return;
    set({ positionMs: ms });
  },

  updateSnapshot: (snapshot) => {
    set({ snapshot, positionMs: snapshot.position_ms });
  },

  loadState: async () => {
    try {
      const snapshot = await api.getPlaybackState();
      set({ snapshot, positionMs: snapshot.position_ms });
    } catch (e) {
      notifyError(e);
    }
  },
}));
