import { useEffect, useCallback, useRef, useState } from "react";
import { listen } from "@tauri-apps/api/event";
import { AppLayout } from "@/components/Layout/AppLayout";
import { LibrarySetup } from "@/components/Settings/LibrarySetup";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import { useKeyboardShortcuts } from "@/hooks/use-keyboard-shortcuts";
import { useFileDrop } from "@/hooks/use-file-drop";
import { notifyError } from "@/lib/errors";
import * as api from "@/lib/tauri";
import type {
  BatchSeparationProgress,
  PlaybackPositionEvent,
  PlaybackEndedEvent,
  SeparationProgressEvent,
  SeparationCompleteEvent,
  SeparationErrorEvent,
  ModelBootstrapStatusSnapshot,
} from "@/types/ipc";

function App() {
  const [libraryReady, setLibraryReady] = useState<boolean | null>(null);
  const loadLibrary = useLibraryStore((s) => s.loadLibrary);
  const loadBootstrapStatus = useBootstrapStore((s) => s.loadStatus);

  // Check if a library is configured on mount
  useEffect(() => {
    api
      .getLibraryPath()
      .then((path) => setLibraryReady(path !== null))
      .catch((e) => {
        notifyError(e);
        setLibraryReady(false);
      });
  }, []);

  // Load initial data once library is ready
  useEffect(() => {
    if (libraryReady) {
      loadLibrary();
      loadBootstrapStatus();
      usePlayerStore.getState().loadState();
    }
  }, [libraryReady, loadLibrary, loadBootstrapStatus]);

  // Set up all Tauri event listeners
  useEventListeners();

  // Auto-fetch lyrics when song changes
  useLyricsAutoFetch();

  // Activate lyrics sync rAF loop
  useLyricsSync();

  // Global keyboard shortcuts
  useKeyboardShortcuts();

  // File drop import
  useFileDrop();

  const handleLibrarySetupComplete = useCallback(() => {
    setLibraryReady(true);
  }, []);

  // Show nothing while checking library state
  if (libraryReady === null) {
    return null;
  }

  // Show setup wizard if no library is configured
  if (!libraryReady) {
    return <LibrarySetup onComplete={handleLibrarySetupComplete} />;
  }

  return <AppLayout />;
}

function useLyricsAutoFetch() {
  const songId = usePlayerStore((s) => s.snapshot?.song_id) ?? undefined;
  const fetchLyrics = useLyricsStore((s) => s.fetchLyrics);
  const prevSongIdRef = useRef<string | undefined>(undefined);

  useEffect(() => {
    if (songId && songId !== prevSongIdRef.current) {
      fetchLyrics(songId);
    }
    prevSongIdRef.current = songId;
  }, [songId, fetchLyrics]);
}

function useEventListeners() {
  const updatePosition = usePlayerStore((s) => s.updatePosition);
  const updateSeparationStatus = useLibraryStore(
    (s) => s.updateSeparationStatus,
  );
  const updateBatchProgress = useLibraryStore((s) => s.updateBatchProgress);
  const clearBatchSeparation = useLibraryStore((s) => s.clearBatchSeparation);
  const updateBootstrapStatus = useBootstrapStore((s) => s.updateStatus);
  const loadStems = usePlayerStore((s) => s.loadStems);

  // Use a ref for currentSongId to avoid re-creating listeners on every song change
  const currentSongIdRef = useRef<string | undefined>(undefined);
  const currentSongId = usePlayerStore((s) => s.snapshot?.song_id) ?? undefined;
  useEffect(() => {
    currentSongIdRef.current = currentSongId;
  }, [currentSongId]);

  useEffect(() => {
    const unlisteners: (() => void)[] = [];
    let cancelled = false;

    const setup = async () => {
      const u1 = await listen<PlaybackPositionEvent>(
        "playback-position",
        (e) => {
          if (!cancelled) updatePosition(e.payload.ms);
        },
      );

      const u2 = await listen<SeparationProgressEvent>(
        "separation-progress",
        (e) => {
          if (!cancelled)
            updateSeparationStatus({
              song_id: e.payload.song_id,
              state: "running",
              percent: e.payload.percent,
              cache_hit: false,
              vocals_path: null,
              accomp_path: null,
              drums_path: null,
              bass_path: null,
              other_path: null,
              error: null,
            });
        },
      );

      const u3 = await listen<SeparationCompleteEvent>(
        "separation-complete",
        (e) => {
          if (cancelled) return;
          // Fetch the real status to get stem paths for display.
          api
            .getSeparationStatus(e.payload.song_id)
            .then((status) => updateSeparationStatus(status))
            .catch(() =>
              updateSeparationStatus({
                song_id: e.payload.song_id,
                state: "completed",
                percent: 100,
                cache_hit: false,
                vocals_path: null,
                accomp_path: null,
                drums_path: null,
                bass_path: null,
                other_path: null,
                error: null,
              }),
            );

          // Auto-load stems if the separated song is currently playing
          if (e.payload.song_id === currentSongIdRef.current) {
            loadStems().catch((err) => notifyError(err));
          }
        },
      );

      const u4 = await listen<SeparationErrorEvent>(
        "separation-error",
        (e) => {
          if (!cancelled) {
            updateSeparationStatus({
              song_id: e.payload.song_id,
              state: "failed",
              percent: 0,
              cache_hit: false,
              vocals_path: null,
              accomp_path: null,
              drums_path: null,
              bass_path: null,
              other_path: null,
              error: e.payload.error,
            });
            notifyError(e.payload.error);
          }
        },
      );

      const u5 = await listen<ModelBootstrapStatusSnapshot>(
        "model-bootstrap-progress",
        (e) => {
          if (!cancelled) updateBootstrapStatus(e.payload);
        },
      );

      const u6 = await listen<ModelBootstrapStatusSnapshot>(
        "model-bootstrap-ready",
        (e) => {
          if (!cancelled) updateBootstrapStatus(e.payload);
        },
      );

      const u7 = await listen<ModelBootstrapStatusSnapshot>(
        "model-bootstrap-error",
        (e) => {
          if (!cancelled) updateBootstrapStatus(e.payload);
        },
      );

      const u8 = await listen<PlaybackEndedEvent>(
        "playback-ended",
        (e) => {
          if (!cancelled) {
            usePlayerStore.getState().playNextFromQueue(e.payload.song_id);
          }
        },
      );

      const u9 = await listen<BatchSeparationProgress>(
        "batch-separation-progress",
        (e) => {
          if (!cancelled) updateBatchProgress(e.payload);
        },
      );

      const u10 = await listen<BatchSeparationProgress>(
        "batch-separation-complete",
        (e) => {
          if (!cancelled) {
            updateBatchProgress(e.payload);
            // Clear batch state after a short delay so the user sees the final state.
            setTimeout(() => clearBatchSeparation(), 3000);
          }
        },
      );

      const u11 = await listen<BatchSeparationProgress>(
        "batch-separation-cancelled",
        (e) => {
          if (!cancelled) {
            updateBatchProgress(e.payload);
            setTimeout(() => clearBatchSeparation(), 3000);
          }
        },
      );

      if (cancelled) {
        [u1, u2, u3, u4, u5, u6, u7, u8, u9, u10, u11].forEach((fn) => fn());
      } else {
        unlisteners.push(u1, u2, u3, u4, u5, u6, u7, u8, u9, u10, u11);
      }
    };

    setup();

    return () => {
      cancelled = true;
      unlisteners.forEach((fn) => fn());
    };
  }, [updatePosition, updateSeparationStatus, updateBatchProgress, clearBatchSeparation, loadStems, updateBootstrapStatus]);
}

export default App;
