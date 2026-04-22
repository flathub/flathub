import { useNotificationStore } from "@/stores/notification-store";
import i18next from "@/lib/i18n";
import type { CommandError, ErrorCode } from "@/types/ipc";

function getErrorTitle(code: ErrorCode): string {
  return i18next.t(`errors.${code}`) || "Error";
}

function isCommandError(err: unknown): err is CommandError {
  if (typeof err !== "object" || err === null) return false;
  const obj = err as Record<string, unknown>;
  return (
    typeof obj.code === "string" &&
    typeof obj.message === "string" &&
    typeof obj.retryable === "boolean"
  );
}

function isErrorWithMessage(err: unknown): err is { message: string } {
  if (typeof err !== "object" || err === null) return false;
  const obj = err as Record<string, unknown>;
  return typeof obj.message === "string";
}

export function getErrorMessage(error: unknown): string {
  if (isCommandError(error)) {
    return error.message;
  }

  if (error instanceof Error) {
    return error.message;
  }

  if (isErrorWithMessage(error)) {
    return error.message;
  }

  return String(error);
}

export function notifyError(error: unknown, retryAction?: () => void) {
  const store = useNotificationStore.getState();

  if (isCommandError(error)) {
    store.addNotification({
      type: "error",
      title: getErrorTitle(error.code),
      message: error.message,
      retryable: error.retryable,
      retryAction: error.retryable ? retryAction : undefined,
      dismissAfterMs: null,
    });
    return;
  }

  const message = getErrorMessage(error);

  store.addNotification({
    type: "error",
    title: i18next.t("errors.somethingWentWrong"),
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
