import { create } from "zustand";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import type { LyricLine, LyricsSource } from "@/types/ipc";

interface LyricsState {
  songId: string | null;
  lines: LyricLine[];
  source: LyricsSource | null;
  offsetMs: number;
  rawLrc: string;
  activeLineIndex: number;
  isLoading: boolean;

  fetchLyrics: (songId: string) => Promise<void>;
  setOffset: (songId: string, ms: number) => Promise<void>;
  adjustOffset: (songId: string, deltaMs: number) => Promise<void>;
  saveManualLyrics: (songId: string, text: string) => Promise<boolean>;
  setActiveLineIndex: (index: number) => void;
  clear: () => void;
}

export const useLyricsStore = create<LyricsState>((set, get) => ({
  songId: null,
  lines: [],
  source: null,
  offsetMs: 0,
  rawLrc: "",
  activeLineIndex: -1,
  isLoading: false,

  fetchLyrics: async (songId) => {
    set({
      isLoading: true,
      lines: [],
      source: null,
      rawLrc: "",
      activeLineIndex: -1,
    });
    try {
      const payload = await api.fetchLyrics(songId);
      set({
        songId: payload.song_id,
        lines: payload.lines,
        source: payload.source,
        offsetMs: payload.offset_ms,
        rawLrc: payload.raw_lrc,
      });

      // Auto-upgrade: if lyrics are unsynced (all time_ms === 0) and not from LrcLib,
      // try fetching synced lyrics from network
      if (
        payload.lines.length > 0 &&
        payload.source !== "lrc_lib" &&
        payload.lines.every((l) => l.time_ms === 0)
      ) {
        try {
          const online = await api.fetchLyricsOnline(songId);
          if (
            online.lines.length > 0 &&
            online.lines.some((l) => l.time_ms > 0)
          ) {
            set({
              songId: online.song_id,
              lines: online.lines,
              source: online.source,
              offsetMs: online.offset_ms,
              rawLrc: online.raw_lrc,
            });
          }
          // If online lyrics are also unsynced or empty, keep the original local lyrics (already set above)
        } catch {
          // Network failure is non-fatal; keep original local lyrics
        }
      }
    } catch (e) {
      notifyError(e);
      set({ lines: [], source: null, rawLrc: "" });
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

  saveManualLyrics: async (songId, text) => {
    try {
      const payload = await api.saveManualLyrics(songId, text);
      set({
        songId: payload.song_id,
        lines: payload.lines,
        source: payload.source,
        offsetMs: payload.offset_ms,
        rawLrc: payload.raw_lrc,
      });
      return true;
    } catch (e) {
      notifyError(e);
      return false;
    }
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
      rawLrc: "",
      activeLineIndex: -1,
    }),
}));
