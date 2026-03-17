import { useEffect, useRef } from "react";
import { listen } from "@tauri-apps/api/event";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import { notifyError } from "@/lib/errors";
import i18next, { detectSystemLanguage } from "@/lib/i18n";
import * as api from "@/lib/tauri";
import type {
  BatchSeparationProgress,
  ModelBootstrapStatusSnapshot,
  PlaybackEndedEvent,
  PlaybackPositionEvent,
  SeparationCompleteEvent,
  SeparationErrorEvent,
  SeparationProgressEvent,
} from "@/types/ipc";

export function useLyricsAutoFetch() {
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

export function useEventListeners() {
  const updatePosition = usePlayerStore((s) => s.updatePosition);
  const updateSeparationStatus = useLibraryStore(
    (s) => s.updateSeparationStatus,
  );
  const updateBatchProgress = useLibraryStore((s) => s.updateBatchProgress);
  const clearBatchSeparation = useLibraryStore((s) => s.clearBatchSeparation);
  const updateBootstrapStatus = useBootstrapStore((s) => s.updateStatus);
  const loadStems = usePlayerStore((s) => s.loadStems);

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
              model_variant: null,
              error: null,
            });
        },
      );

      const u3 = await listen<SeparationCompleteEvent>(
        "separation-complete",
        (e) => {
          if (cancelled) return;
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
                model_variant: null,
                error: null,
              }),
            );

          if (e.payload.song_id === currentSongIdRef.current) {
            loadStems().catch((err) => notifyError(err));
          }
        },
      );

      const u4 = await listen<SeparationErrorEvent>("separation-error", (e) => {
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
            model_variant: null,
            error: e.payload.error,
          });
          notifyError(e.payload.error);
        }
      });

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

      const u8 = await listen<PlaybackEndedEvent>("playback-ended", (e) => {
        if (!cancelled) {
          usePlayerStore.getState().playNextFromQueue(e.payload.song_id);
        }
      });

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
  }, [
    updatePosition,
    updateSeparationStatus,
    updateBatchProgress,
    clearBatchSeparation,
    loadStems,
    updateBootstrapStatus,
  ]);
}

export function useFullscreenPlaybackRuntime() {
  const updatePosition = usePlayerStore((s) => s.updatePosition);
  const updateSnapshot = usePlayerStore((s) => s.updateSnapshot);

  useEffect(() => {
    api
      .getPlaybackState()
      .then((snapshot) => updateSnapshot(snapshot))
      .catch(() => {});

    api
      .getSettings()
      .then((settings) => {
        const lang = settings.language ?? detectSystemLanguage();
        i18next.changeLanguage(lang);
      })
      .catch(() => {});
  }, [updateSnapshot]);

  useEffect(() => {
    let cancelled = false;
    let unlisten: (() => void) | null = null;

    const setup = async () => {
      unlisten = await listen<PlaybackPositionEvent>(
        "playback-position",
        (e) => {
          if (!cancelled) updatePosition(e.payload.ms);
        },
      );
    };

    setup();

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [updatePosition]);

  useEffect(() => {
    const interval = window.setInterval(() => {
      api
        .getPlaybackState()
        .then((snapshot) => updateSnapshot(snapshot))
        .catch(() => {});
    }, 500);

    return () => {
      window.clearInterval(interval);
    };
  }, [updateSnapshot]);
}
