import type {
  SeparationErrorEvent,
  SeparationProgressEvent,
  SeparationStatusSnapshot,
  UploadCompleteEvent,
  UploadErrorEvent,
  UploadProgressEvent,
  UploadStatusSnapshot,
} from "@/types/ipc";

export const EVENT_STATUS_CLEAR_DELAY_MS = 3_000;

export function separationProgressStatus(
  event: SeparationProgressEvent,
): SeparationStatusSnapshot {
  return {
    song_id: event.song_id,
    state: "running",
    percent: event.percent,
    cache_hit: false,
    vocals_path: null,
    accomp_path: null,
    drums_path: null,
    bass_path: null,
    other_path: null,
    model_variant: null,
    error: null,
  };
}

export function separationErrorStatus(
  event: SeparationErrorEvent,
): SeparationStatusSnapshot {
  return {
    song_id: event.song_id,
    state: "failed",
    percent: 0,
    cache_hit: false,
    vocals_path: null,
    accomp_path: null,
    drums_path: null,
    bass_path: null,
    other_path: null,
    model_variant: null,
    error: event.error,
  };
}

export function uploadProgressStatus(
  event: UploadProgressEvent,
): UploadStatusSnapshot {
  return {
    song_id: event.song_id,
    state: "running",
    percent: event.percent,
    remote_library_id: event.remote_library_id,
    detail: event.detail,
    error: null,
  };
}

export function uploadCompleteStatus(
  event: UploadCompleteEvent,
): UploadStatusSnapshot {
  return {
    song_id: event.song_id,
    state: "completed",
    percent: 100,
    remote_library_id: event.remote_library_id,
    detail: null,
    error: null,
  };
}

export function uploadErrorStatus(
  event: UploadErrorEvent,
): UploadStatusSnapshot {
  return {
    song_id: event.song_id,
    state: "failed",
    percent: 0,
    remote_library_id: event.remote_library_id,
    detail: null,
    error: event.error,
  };
}

export function createStatusClearScheduler<K>(
  clear: (key: K) => void,
  delayMs = EVENT_STATUS_CLEAR_DELAY_MS,
) {
  const timers = new Map<K, ReturnType<typeof setTimeout>>();

  const cancel = (key: K) => {
    const timer = timers.get(key);
    if (timer != null) {
      globalThis.clearTimeout(timer);
      timers.delete(key);
    }
  };

  return {
    cancel,
    schedule(key: K) {
      cancel(key);
      timers.set(
        key,
        globalThis.setTimeout(() => {
          timers.delete(key);
          clear(key);
        }, delayMs),
      );
    },
    clearAll() {
      timers.forEach((timer) => globalThis.clearTimeout(timer));
      timers.clear();
    },
  };
}

export function createBatchSeparationClearScheduler(
  clearBatchSeparation: () => void,
  delayMs = EVENT_STATUS_CLEAR_DELAY_MS,
) {
  const scheduler = createStatusClearScheduler<"batch">(
    () => clearBatchSeparation(),
    delayMs,
  );

  return {
    scheduleAfterTerminalProgress() {
      scheduler.schedule("batch");
    },
    clearAll: scheduler.clearAll,
  };
}
