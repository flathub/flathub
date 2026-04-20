import { create } from "zustand";
import * as api from "@/lib/tauri";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";
import { invalidateCoverArtUrl } from "@/lib/cover-art";
import { notifyError } from "@/lib/errors";
import {
  buildAmbiguousCdgChoiceRequests,
  buildImportSongsOptions,
  type AmbiguousCdgChoiceRequest,
  type ExplicitCdgSelection,
} from "@/lib/import-cdg-selection";
import type {
  BatchSeparationProgress,
  ImportFailure,
  SeparationStatusSnapshot,
  Song,
  UploadStatusSnapshot,
} from "@/types/ipc";

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
      const explicitSelections: ExplicitCdgSelection[] = [];
      const excludedAmbiguousAudioPaths = new Set<string>();

      for (const request of buildAmbiguousCdgChoiceRequests(audioPaths)) {
        const selectedAudioPath = await get().promptForCdgChoice(request);
        if (selectedAudioPath) {
          for (const candidate of request.audioCandidates) {
            if (candidate !== selectedAudioPath) {
              excludedAmbiguousAudioPaths.add(candidate);
            }
          }
          explicitSelections.push({
            audioPath: selectedAudioPath,
            cdgPath: request.cdgPath,
          });
        }
      }

      const audioPathsToImport = audioPaths.filter(
        (path) => !excludedAmbiguousAudioPaths.has(path),
      );

      // Import audio files
      if (audioPathsToImport.length > 0) {
        const result = await api.importSongs(
          audioPathsToImport,
          buildImportSongsOptions(explicitSelections),
        );
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
      publishLibraryInvalidation();
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
        return {};
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
