import { create } from "zustand";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import type { LyricLine, LyricsSource } from "@/types/ipc";

interface LyricsState {
  songId: string | null;
  lines: LyricLine[];
  source: LyricsSource | null;
  offsetMs: number;
  activeLineIndex: number;
  isLoading: boolean;

  fetchLyrics: (songId: string) => Promise<void>;
  setOffset: (songId: string, ms: number) => Promise<void>;
  adjustOffset: (songId: string, deltaMs: number) => Promise<void>;
  setActiveLineIndex: (index: number) => void;
  clear: () => void;
}

export const useLyricsStore = create<LyricsState>((set, get) => ({
  songId: null,
  lines: [],
  source: null,
  offsetMs: 0,
  activeLineIndex: -1,
  isLoading: false,

  fetchLyrics: async (songId) => {
    set({ isLoading: true, lines: [], source: null, activeLineIndex: -1 });
    try {
      const payload = await api.fetchLyrics(songId);
      set({
        songId: payload.song_id,
        lines: payload.lines,
        source: payload.source,
        offsetMs: payload.offset_ms,
      });
    } catch (e) {
      notifyError(e);
      set({ lines: [], source: null });
    } finally {
      set({ isLoading: false });
    }
  },

  setOffset: async (songId, ms) => {
    await api.setLyricsOffset(songId, ms);
    set({ offsetMs: ms });
  },

  adjustOffset: async (songId, deltaMs) => {
    const newOffset = get().offsetMs + deltaMs;
    await api.setLyricsOffset(songId, newOffset);
    set({ offsetMs: newOffset });
  },

  setActiveLineIndex: (index) => {
    if (index !== get().activeLineIndex) {
      set({ activeLineIndex: index });
    }
  },

  clear: () =>
    set({
      songId: null,
      lines: [],
      source: null,
      offsetMs: 0,
      activeLineIndex: -1,
    }),
}));
