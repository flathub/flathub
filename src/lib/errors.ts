import { useNotificationStore } from "@/stores/notification-store";
import type { CommandError, ErrorCode } from "@/types/ipc";

const ERROR_TITLES: Record<ErrorCode, string> = {
  database_unavailable: "Database Error",
  media_read_failed: "Import Failed",
  song_not_found: "Song Not Found",
  model_unavailable: "Model Unavailable",
  audio_decode_failed: "Audio Decode Error",
  audio_output_unavailable: "Audio Output Error",
  karaoke_not_ready: "Karaoke Not Ready",
  lyrics_not_ready: "Lyrics Unavailable",
  network_unavailable: "Network Error",
  invalid_playback_state: "Playback Error",
  separation_failed: "Separation Failed",
  internal: "Internal Error",
};

function isCommandError(err: unknown): err is CommandError {
  if (typeof err !== "object" || err === null) return false;
  const obj = err as Record<string, unknown>;
  return (
    typeof obj.code === "string" &&
    typeof obj.message === "string" &&
    typeof obj.retryable === "boolean"
  );
}

export function notifyError(
  error: unknown,
  retryAction?: () => void,
) {
  const store = useNotificationStore.getState();

  if (isCommandError(error)) {
    store.addNotification({
      type: "error",
      title: ERROR_TITLES[error.code] ?? "Error",
      message: error.message,
      retryable: error.retryable,
      retryAction: error.retryable ? retryAction : undefined,
      dismissAfterMs: null,
    });
    return;
  }

  const message =
    error instanceof Error ? error.message : String(error);

  store.addNotification({
    type: "error",
    title: "Something went wrong",
    message,
    retryable: false,
    dismissAfterMs: null,
  });
}

export function notifySuccess(title: string, message: string = "") {
  useNotificationStore.getState().addNotification({
    type: "success",
    title,
    message,
    retryable: false,
    dismissAfterMs: 4000,
  });
}
