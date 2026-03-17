import { create } from "zustand";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import type {
  BatchSeparationProgress,
  ImportFailure,
  SeparationStatusSnapshot,
  Song,
} from "@/types/ipc";

interface LibraryState {
  songs: Song[];
  searchQuery: string;
  isImporting: boolean;
  importErrors: ImportFailure[];
  selectedSongIds: Set<string>;
  lastClickedSongId: string | null;
  separationStatuses: Record<string, SeparationStatusSnapshot>;
  filter: "all" | "separated";
  batchSeparation: BatchSeparationProgress | null;
  hideBatchSeparate: boolean;

  loadLibrary: () => Promise<void>;
  importFiles: (paths: string[]) => Promise<void>;
  setSearchQuery: (query: string) => void;
  searchSongs: (query: string) => Promise<void>;
  selectSong: (
    songId: string,
    event: { shiftKey: boolean; metaKey: boolean; ctrlKey: boolean },
    orderedHashes?: string[],
  ) => void;
  clearSelection: () => void;
  setFilter: (filter: "all" | "separated") => void;
  updateSongMetadata: (
    hash: string,
    title: string | null,
    artist: string | null,
  ) => Promise<boolean>;
  updateSeparationStatus: (status: SeparationStatusSnapshot) => void;
  clearAllSeparationStatuses: () => void;
  updateBatchProgress: (progress: BatchSeparationProgress) => void;
  clearBatchSeparation: () => void;
  clearImportErrors: () => void;
  setHideBatchSeparate: (value: boolean) => void;
}

export const useLibraryStore = create<LibraryState>((set, get) => ({
  songs: [],
  searchQuery: "",
  isImporting: false,
  importErrors: [],
  selectedSongIds: new Set<string>(),
  lastClickedSongId: null,
  separationStatuses: {},
  filter: "all",
  batchSeparation: null,
  hideBatchSeparate: false,

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
      // Split paths into audio and lyrics files
      const audioPaths = paths.filter((p) => !p.toLowerCase().endsWith(".lrc"));
      const lrcPaths = paths.filter((p) => p.toLowerCase().endsWith(".lrc"));

      // Import audio files
      if (audioPaths.length > 0) {
        const result = await api.importSongs(audioPaths);
        if (result.failed.length > 0) {
          set({ importErrors: result.failed });
          for (const failure of result.failed) {
            notifyError(failure.error);
          }
        }
      }

      // Import LRC files (must happen after audio so songs exist for matching)
      if (lrcPaths.length > 0) {
        const lrcResult = await api.importLyricsFiles(lrcPaths);
        void lrcResult;
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

  selectSong: (songId, event, orderedHashes) => {
    const { selectedSongIds, lastClickedSongId } = get();

    if (event.shiftKey && lastClickedSongId && orderedHashes) {
      // Range selection
      const startIdx = orderedHashes.indexOf(lastClickedSongId);
      const endIdx = orderedHashes.indexOf(songId);
      if (startIdx !== -1 && endIdx !== -1) {
        const from = Math.min(startIdx, endIdx);
        const to = Math.max(startIdx, endIdx);
        const rangeIds = orderedHashes.slice(from, to + 1);
        const newSet = new Set(selectedSongIds);
        for (const id of rangeIds) {
          newSet.add(id);
        }
        set({ selectedSongIds: newSet });
      }
    } else if (event.metaKey || event.ctrlKey) {
      // Toggle selection
      const newSet = new Set(selectedSongIds);
      if (newSet.has(songId)) {
        newSet.delete(songId);
      } else {
        newSet.add(songId);
      }
      set({ selectedSongIds: newSet, lastClickedSongId: songId });
    } else {
      // Normal click: select only this one
      set({
        selectedSongIds: new Set([songId]),
        lastClickedSongId: songId,
      });
    }
  },

  clearSelection: () =>
    set({ selectedSongIds: new Set(), lastClickedSongId: null }),

  setFilter: (filter) => set({ filter }),

  updateSongMetadata: async (hash, title, artist) => {
    try {
      const updated = await api.updateSongMetadata(hash, title, artist);
      set((state) => ({
        songs: state.songs.map((s) =>
          s.hash === hash
            ? { ...s, title: updated.title, artist: updated.artist }
            : s,
        ),
      }));
      return true;
    } catch (e) {
      notifyError(e);
      return false;
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

  clearAllSeparationStatuses: () => set({ separationStatuses: {} }),

  updateBatchProgress: (progress) => set({ batchSeparation: progress }),

  clearBatchSeparation: () => set({ batchSeparation: null }),

  clearImportErrors: () => set({ importErrors: [] }),

  setHideBatchSeparate: (value) => set({ hideBatchSeparate: value }),
}));
