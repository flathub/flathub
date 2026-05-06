import { create } from "zustand";
import * as api from "@/lib/tauri";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";
import { invalidateCoverArtUrl } from "@/lib/cover-art";
import { notifyError } from "@/lib/errors";
import { runImportWorkflow } from "@/runtime/import-workflow";
import type { AmbiguousCdgChoiceRequest } from "@/lib/import-cdg-selection";
import type {
  BatchSeparationProgress,
  ImportFailure,
  SeparationStatusSnapshot,
  Song,
  UploadStatusSnapshot,
} from "@/types/ipc";

function debounce<T extends (...args: never[]) => void>(fn: T, ms: number): T {
  let timer: ReturnType<typeof setTimeout> | null = null;
  return ((...args: Parameters<T>) => {
    if (timer) clearTimeout(timer);
    timer = setTimeout(() => fn(...args), ms);
  }) as T;
}

interface LibraryState {
  songs: Song[];
  searchQuery: string;
  isImporting: boolean;
  importErrors: ImportFailure[];
  selectedSongIds: Set<string>;
  lastClickedSongId: string | null;
  separationStatuses: Record<string, SeparationStatusSnapshot>;
  uploadStatuses: Record<string, UploadStatusSnapshot>;
  filter: "all" | "separated";
  batchSeparation: BatchSeparationProgress | null;
  pendingImportCdgChoice: AmbiguousCdgChoiceRequest | null;

  loadLibrary: () => Promise<void>;
  importFiles: (paths: string[]) => Promise<void>;
  promptForCdgChoice: (
    request: AmbiguousCdgChoiceRequest,
  ) => Promise<string | null>;
  resolveCdgChoicePrompt: (audioPath: string | null) => void;
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
  setSongsInstrumental: (
    songIds: string[],
    instrumental: boolean,
  ) => Promise<boolean>;
  extractEmbeddedCoverArt: (songIds: string[]) => Promise<boolean>;
  updateSeparationStatus: (status: SeparationStatusSnapshot) => void;
  clearAllSeparationStatuses: () => void;
  updateUploadStatus: (status: UploadStatusSnapshot) => void;
  clearUploadStatus: (songId: string) => void;
  clearAllUploadStatuses: () => void;
  updateBatchProgress: (progress: BatchSeparationProgress) => void;
  clearBatchSeparation: () => void;
  clearImportErrors: () => void;
}

let pendingCdgChoiceResolver: ((audioPath: string | null) => void) | null =
  null;

const librarySyncChannel = createWebviewSyncChannel<{ revision: number }>(
  "openkara.library",
);
let librarySyncRevision = 0;

function publishLibraryInvalidation() {
  librarySyncRevision += 1;
  librarySyncChannel.publish({ revision: librarySyncRevision });
}

const debouncedSearch = debounce(async (query: string) => {
  try {
    const songs = await api.searchLibrary(query);
    useLibraryStore.setState({ songs });
  } catch (e) {
    notifyError(e);
  }
}, 300);

export const useLibraryStore = create<LibraryState>((set, get) => ({
  songs: [],
  searchQuery: "",
  isImporting: false,
  importErrors: [],
  selectedSongIds: new Set<string>(),
  lastClickedSongId: null,
  separationStatuses: {},
  uploadStatuses: {},
  filter: "all",
  batchSeparation: null,
  pendingImportCdgChoice: null,

  loadLibrary: async () => {
    try {
      try {
        const activeLibrary = await api.getActiveLibrary();
        if (activeLibrary?.kind === "remote") {
          await api.refreshRemoteRepository();
        }
      } catch {
        // Keep stale cache visible if the repository refresh attempt fails.
      }

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

      // Hydrate upload statuses so in-progress uploads survive app restart.
      try {
        const uploads = await api.getAllUploadStatuses();
        const uploadMap: Record<string, UploadStatusSnapshot> = {};
        for (const s of uploads) {
          uploadMap[s.song_id] = s;
        }
        set({ uploadStatuses: uploadMap });
      } catch {
        // Non-fatal: upload statuses will remain empty
      }
    } catch (e) {
      notifyError(e);
    }
  },

  importFiles: async (paths) => {
    set({ isImporting: true, importErrors: [] });
    try {
      await runImportWorkflow({
        paths,
        api,
        promptForCdgChoice: get().promptForCdgChoice,
        notifyError,
        setImportErrors: (importErrors) => set({ importErrors }),
        setSongs: (songs) => set({ songs }),
        publishLibraryInvalidation,
      });
    } catch (e) {
      notifyError(e);
    } finally {
      set({ isImporting: false });
    }
  },

  promptForCdgChoice: async (request) => {
    set({ pendingImportCdgChoice: request });

    return new Promise((resolve) => {
      pendingCdgChoiceResolver = resolve;
    });
  },

  resolveCdgChoicePrompt: (audioPath) => {
    set({ pendingImportCdgChoice: null });
    pendingCdgChoiceResolver?.(audioPath);
    pendingCdgChoiceResolver = null;
  },

  setSearchQuery: (query) => {
    set({ searchQuery: query });
    if (query.trim()) {
      debouncedSearch(query);
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
      publishLibraryInvalidation();
      return true;
    } catch (e) {
      notifyError(e);
      return false;
    }
  },

  setSongsInstrumental: async (songIds, instrumental) => {
    try {
      const updatedSongs = await api.setSongsInstrumental(
        songIds,
        instrumental,
      );
      const updatedByHash = new Map(
        updatedSongs.map((song) => [song.hash, song]),
      );

      set((state) => ({
        songs: state.songs.map((song) => updatedByHash.get(song.hash) ?? song),
      }));

      publishLibraryInvalidation();

      return true;
    } catch (e) {
      notifyError(e);
      return false;
    }
  },

  extractEmbeddedCoverArt: async (songIds) => {
    try {
      const result = await api.extractEmbeddedCoverArt(songIds);

      for (const song of result.updated_songs) {
        invalidateCoverArtUrl(song.hash);
      }

      if (result.updated_songs.length > 0) {
        const updatedByHash = new Map(
          result.updated_songs.map((song) => [song.hash, song]),
        );
        set((state) => ({
          songs: state.songs.map(
            (song) => updatedByHash.get(song.hash) ?? song,
          ),
        }));
        publishLibraryInvalidation();
      }

      for (const failure of result.failed) {
        notifyError(failure.error);
      }

      return result.updated_songs.length > 0;
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

  updateUploadStatus: (status) => {
    set((state) => ({
      uploadStatuses: {
        ...state.uploadStatuses,
        [status.song_id]: status,
      },
    }));
  },

  clearUploadStatus: (songId) =>
    set((state) => {
      if (!(songId in state.uploadStatuses)) {
        return state;
      }

      const next = { ...state.uploadStatuses };
      delete next[songId];
      return { uploadStatuses: next };
    }),

  clearAllUploadStatuses: () => set({ uploadStatuses: {} }),

  updateBatchProgress: (progress) => set({ batchSeparation: progress }),

  clearBatchSeparation: () => set({ batchSeparation: null }),

  clearImportErrors: () => set({ importErrors: [] }),
}));

librarySyncChannel.subscribe(() => {
  void useLibraryStore.getState().loadLibrary();
});
