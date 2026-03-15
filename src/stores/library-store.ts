import { create } from "zustand";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import type {
  ImportFailure,
  SeparationStatusSnapshot,
  Song,
} from "@/types/ipc";

interface LibraryState {
  songs: Song[];
  searchQuery: string;
  isImporting: boolean;
  importErrors: ImportFailure[];
  selectedSongId: string | null;
  separationStatuses: Record<string, SeparationStatusSnapshot>;
  filter: "all" | "separated";

  loadLibrary: () => Promise<void>;
  importFiles: (paths: string[]) => Promise<void>;
  setSearchQuery: (query: string) => void;
  searchSongs: (query: string) => Promise<void>;
  setSelectedSongId: (id: string | null) => void;
  setFilter: (filter: "all" | "separated") => void;
  updateSongMetadata: (
    hash: string,
    title: string | null,
    artist: string | null,
  ) => Promise<void>;
  updateSeparationStatus: (status: SeparationStatusSnapshot) => void;
  clearImportErrors: () => void;
}

export const useLibraryStore = create<LibraryState>((set, get) => ({
  songs: [],
  searchQuery: "",
  isImporting: false,
  importErrors: [],
  selectedSongId: null,
  separationStatuses: {},
  filter: "all",

  loadLibrary: async () => {
    try {
      const songs = await api.getLibrary();
      set({ songs });

      // Hydrate separation statuses from the database so previously
      // separated songs show as "completed" after app restart.
      try {
        const statuses = await api.getAllSeparationStatuses();
        const statusMap: Record<string, SeparationStatusSnapshot> = {};
        for (const s of statuses) {
          statusMap[s.song_id] = s;
        }
        set({ separationStatuses: statusMap });
      } catch {
        // Non-fatal: separation statuses will remain empty
      }
    } catch (e) {
      notifyError(e);
    }
  },

  importFiles: async (paths) => {
    set({ isImporting: true, importErrors: [] });
    try {
      const result = await api.importSongs(paths);
      if (result.failed.length > 0) {
        set({ importErrors: result.failed });
        for (const failure of result.failed) {
          notifyError(failure.error);
        }
      }
      // Reload full library to get consistent state
      const songs = await api.getLibrary();
      set({ songs });
    } catch (e) {
      notifyError(e);
    } finally {
      set({ isImporting: false });
    }
  },

  setSearchQuery: (query) => {
    set({ searchQuery: query });
    if (query.trim()) {
      get().searchSongs(query);
    } else {
      get().loadLibrary();
    }
  },

  searchSongs: async (query) => {
    try {
      const songs = await api.searchLibrary(query);
      set({ songs });
    } catch (e) {
      notifyError(e);
    }
  },

  setSelectedSongId: (id) => set({ selectedSongId: id }),

  setFilter: (filter) => set({ filter }),

  updateSongMetadata: async (hash, title, artist) => {
    try {
      const updated = await api.updateSongMetadata(hash, title, artist);
      set((state) => ({
        songs: state.songs.map((s) =>
          s.hash === hash ? { ...s, title: updated.title, artist: updated.artist } : s,
        ),
      }));
    } catch (e) {
      notifyError(e);
    }
  },

  updateSeparationStatus: (status) => {
    set((state) => ({
      separationStatuses: {
        ...state.separationStatuses,
        [status.song_id]: status,
      },
    }));
  },

  clearImportErrors: () => set({ importErrors: [] }),
}));
