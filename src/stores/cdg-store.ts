import { create } from "zustand";

interface CdgState {
  /** Whether the currently playing song has a CDG file. */
  hasCdg: boolean;
  songId: string | null;

  setSong: (songId: string | null, hasCdg: boolean) => void;
  clear: () => void;
}

export const useCdgStore = create<CdgState>((set) => ({
  hasCdg: false,
  songId: null,

  setSong: (songId, hasCdg) => set({ songId, hasCdg }),
  clear: () => set({ hasCdg: false, songId: null }),
}));
