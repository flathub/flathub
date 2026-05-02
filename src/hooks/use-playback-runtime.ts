import { useEffect, useRef } from "react";
import { listen } from "@tauri-apps/api/event";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import { useSettingsStore } from "@/stores/settings-store";
import { notifyError } from "@/lib/errors";
import i18next, { detectSystemLanguage } from "@/lib/i18n";
import * as api from "@/lib/tauri";
import {
  createBatchSeparationClearScheduler,
  createStatusClearScheduler,
  fallbackSeparationCompleteStatus,
  separationErrorStatus,
  separationProgressStatus,
  uploadCompleteStatus,
  uploadErrorStatus,
  uploadProgressStatus,
} from "@/runtime/event-reducers";
import { loadStartupSettings } from "@/runtime/settings-runtime";
import type {
  BatchSeparationProgress,
  ModelBootstrapStatusSnapshot,
  PlaybackEndedEvent,
  PlaybackPositionEvent,
  SeparationCompleteEvent,
  SeparationErrorEvent,
  SeparationProgressEvent,
  UploadCompleteEvent,
  UploadErrorEvent,
  UploadProgressEvent,
} from "@/types/ipc";

export function useLyricsAutoFetch(enabled = true) {
  const songId = usePlayerStore((s) => s.snapshot?.song_id) ?? undefined;
  const fetchLyrics = useLyricsStore((s) => s.fetchLyrics);
  const prevSongIdRef = useRef<string | undefined>(undefined);

  useEffect(() => {
    if (!enabled) {
      prevSongIdRef.current = undefined;
      return;
    }

    if (songId && songId !== prevSongIdRef.current) {
      fetchLyrics(songId);
    }
    prevSongIdRef.current = songId;
  }, [enabled, songId, fetchLyrics]);
}

function usePlaybackPositionSubscription(
  enabled: boolean,
  onPosition: (event: PlaybackPositionEvent) => void,
) {
  useEffect(() => {
    if (!enabled) {
      return;
    }

    let cancelled = false;
    let unlisten: (() => void) | null = null;

    const setup = async () => {
      unlisten = await listen<PlaybackPositionEvent>(
        "playback-position",
        (e) => {
          if (!cancelled) onPosition(e.payload);
        },
      );
    };

    void setup();

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [enabled, onPosition]);
}

function usePlaybackPositionEvents(enabled: boolean) {
  const updatePosition = usePlayerStore((s) => s.updatePosition);
  usePlaybackPositionSubscription(enabled, (event) => updatePosition(event.ms));
}

function useSeparationEvents(enabled: boolean) {
  const updateSeparationStatus = useLibraryStore(
    (s) => s.updateSeparationStatus,
  );
  const loadStems = usePlayerStore((s) => s.loadStems);

  const currentSongIdRef = useRef<string | undefined>(undefined);
  const currentSongId = usePlayerStore((s) => s.snapshot?.song_id) ?? undefined;

  useEffect(() => {
    if (!enabled) {
      currentSongIdRef.current = undefined;
      return;
    }

    currentSongIdRef.current = currentSongId;
  }, [enabled, currentSongId]);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    const unlisteners: (() => void)[] = [];
    let cancelled = false;

    const setup = async () => {
      const progressUnlisten = await listen<SeparationProgressEvent>(
        "separation-progress",
        (e) => {
          if (!cancelled) {
            updateSeparationStatus(separationProgressStatus(e.payload));
          }
        },
      );

      const completeUnlisten = await listen<SeparationCompleteEvent>(
        "separation-complete",
        (e) => {
          if (cancelled) return;
          api
            .getSeparationStatus(e.payload.song_id)
            .then((status) => updateSeparationStatus(status))
            .catch(() =>
              updateSeparationStatus(
                fallbackSeparationCompleteStatus(e.payload.song_id),
              ),
            );

          if (e.payload.song_id === currentSongIdRef.current) {
            loadStems().catch((err) => notifyError(err));
          }
        },
      );

      const errorUnlisten = await listen<SeparationErrorEvent>(
        "separation-error",
        (e) => {
          if (!cancelled) {
            updateSeparationStatus(separationErrorStatus(e.payload));
            notifyError(e.payload.error);
          }
        },
      );

      if (cancelled) {
        progressUnlisten();
        completeUnlisten();
        errorUnlisten();
      } else {
        unlisteners.push(progressUnlisten, completeUnlisten, errorUnlisten);
      }
    };

    void setup();

    return () => {
      cancelled = true;
      unlisteners.forEach((fn) => fn());
    };
  }, [enabled, loadStems, updateSeparationStatus]);
}

function useBootstrapEvents(enabled: boolean) {
  const updateBootstrapStatus = useBootstrapStore((s) => s.updateStatus);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    const unlisteners: (() => void)[] = [];
    let cancelled = false;

    const setup = async () => {
      const progressUnlisten = await listen<ModelBootstrapStatusSnapshot>(
        "model-bootstrap-progress",
        (e) => {
          if (!cancelled) updateBootstrapStatus(e.payload);
        },
      );
      const readyUnlisten = await listen<ModelBootstrapStatusSnapshot>(
        "model-bootstrap-ready",
        (e) => {
          if (!cancelled) updateBootstrapStatus(e.payload);
        },
      );
      const errorUnlisten = await listen<ModelBootstrapStatusSnapshot>(
        "model-bootstrap-error",
        (e) => {
          if (!cancelled) updateBootstrapStatus(e.payload);
        },
      );

      if (cancelled) {
        progressUnlisten();
        readyUnlisten();
        errorUnlisten();
      } else {
        unlisteners.push(progressUnlisten, readyUnlisten, errorUnlisten);
      }
    };

    void setup();

    return () => {
      cancelled = true;
      unlisteners.forEach((fn) => fn());
    };
  }, [enabled, updateBootstrapStatus]);
}

function usePlaybackEndedQueueAdvance(enabled: boolean) {
  useEffect(() => {
    if (!enabled) {
      return;
    }

    let cancelled = false;
    let unlisten: (() => void) | null = null;

    const setup = async () => {
      unlisten = await listen<PlaybackEndedEvent>("playback-ended", (e) => {
        if (!cancelled) {
          usePlayerStore.getState().playNextFromQueue(e.payload.song_id);
        }
      });
    };

    void setup();

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [enabled]);
}

function useBatchSeparationEvents(enabled: boolean) {
  const updateBatchProgress = useLibraryStore((s) => s.updateBatchProgress);
  const clearBatchSeparation = useLibraryStore((s) => s.clearBatchSeparation);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    const unlisteners: (() => void)[] = [];
    let cancelled = false;
    const clearScheduler =
      createBatchSeparationClearScheduler(clearBatchSeparation);

    const setup = async () => {
      const progressUnlisten = await listen<BatchSeparationProgress>(
        "batch-separation-progress",
        (e) => {
          if (!cancelled) updateBatchProgress(e.payload);
        },
      );
      const completeUnlisten = await listen<BatchSeparationProgress>(
        "batch-separation-complete",
        (e) => {
          if (!cancelled) {
            updateBatchProgress(e.payload);
            clearScheduler.scheduleAfterTerminalProgress();
          }
        },
      );
      const cancelledUnlisten = await listen<BatchSeparationProgress>(
        "batch-separation-cancelled",
        (e) => {
          if (!cancelled) {
            updateBatchProgress(e.payload);
            clearScheduler.scheduleAfterTerminalProgress();
          }
        },
      );

      if (cancelled) {
        progressUnlisten();
        completeUnlisten();
        cancelledUnlisten();
      } else {
        unlisteners.push(progressUnlisten, completeUnlisten, cancelledUnlisten);
      }
    };

    void setup();

    return () => {
      cancelled = true;
      clearScheduler.clearAll();
      unlisteners.forEach((fn) => fn());
    };
  }, [enabled, clearBatchSeparation, updateBatchProgress]);
}

function useUploadEvents(enabled: boolean) {
  const updateUploadStatus = useLibraryStore((s) => s.updateUploadStatus);
  const clearUploadStatus = useLibraryStore((s) => s.clearUploadStatus);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    const unlisteners: (() => void)[] = [];
    let cancelled = false;
    const clearScheduler =
      createStatusClearScheduler<string>(clearUploadStatus);

    const setup = async () => {
      const progressUnlisten = await listen<UploadProgressEvent>(
        "upload-progress",
        (e) => {
          if (cancelled) return;
          const songId = e.payload.song_id;
          clearScheduler.cancel(songId);
          updateUploadStatus(uploadProgressStatus(e.payload));
        },
      );

      const completeUnlisten = await listen<UploadCompleteEvent>(
        "upload-complete",
        (e) => {
          if (cancelled) return;
          const songId = e.payload.song_id;
          updateUploadStatus(uploadCompleteStatus(e.payload));
          clearScheduler.schedule(songId);
        },
      );

      const errorUnlisten = await listen<UploadErrorEvent>(
        "upload-error",
        (e) => {
          if (cancelled) return;
          clearScheduler.cancel(e.payload.song_id);
          updateUploadStatus(uploadErrorStatus(e.payload));
          notifyError(e.payload.error);
        },
      );

      if (cancelled) {
        progressUnlisten();
        completeUnlisten();
        errorUnlisten();
      } else {
        unlisteners.push(progressUnlisten, completeUnlisten, errorUnlisten);
      }
    };

    void setup();

    return () => {
      cancelled = true;
      clearScheduler.clearAll();
      unlisteners.forEach((fn) => fn());
    };
  }, [clearUploadStatus, enabled, updateUploadStatus]);
}

export function useEventListeners(enabled = true) {
  usePlaybackPositionEvents(enabled);
  useSeparationEvents(enabled);
  useBootstrapEvents(enabled);
  usePlaybackEndedQueueAdvance(enabled);
  useBatchSeparationEvents(enabled);
  useUploadEvents(enabled);
}

export function useFullscreenPlaybackRuntime() {
  const updatePosition = usePlayerStore((s) => s.updatePosition);
  const updateSnapshot = usePlayerStore((s) => s.updateSnapshot);
  const hydrateAppSettings = useSettingsStore((s) => s.hydrateAppSettings);

  useEffect(() => {
    void api
      .getPlaybackState()
      .then((snapshot) => updateSnapshot(snapshot))
      .catch(() => {});

    void loadStartupSettings({
      getSettings: api.getSettings,
      hydrateAppSettings,
      changeLanguage: i18next.changeLanguage,
      detectFallbackLanguage: detectSystemLanguage,
    }).catch(() => {});
  }, [hydrateAppSettings, updateSnapshot]);

  usePlaybackPositionSubscription(true, (event) => updatePosition(event.ms));

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
