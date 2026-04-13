import { useEffect, useRef } from "react";
import { createPortal } from "react-dom";
import { useTranslation } from "react-i18next";

interface ConfirmationDialogProps {
  title: string;
  message: string;
  detail?: string;
  confirmLabel: string;
  onConfirm: () => void;
  onCancel: () => void;
}

export function ConfirmationDialog({
  title,
  message,
  detail,
  confirmLabel,
  onConfirm,
  onCancel,
}: ConfirmationDialogProps) {
  const { t } = useTranslation();
  const cancelRef = useRef<HTMLButtonElement>(null);

  useEffect(() => {
    cancelRef.current?.focus();
  }, []);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        onCancel();
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [onCancel]);

  if (typeof document === "undefined" || !document.body) {
    return null;
  }

  return createPortal(
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4">
      {/* Backdrop */}
      <div className="absolute inset-0 bg-black/60" onClick={onCancel} />

      {/* Dialog */}
      <div
        role="dialog"
        aria-modal="true"
        aria-labelledby="confirmation-dialog-title"
        className="relative w-full max-w-sm rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-6 shadow-xl"
      >
        <h3
          id="confirmation-dialog-title"
          className="break-words text-[15px] font-semibold text-white"
        >
          {title}
        </h3>
        <p className="mt-2 break-words text-[13px] text-[var(--color-text-dim)]">
          {message}
        </p>
        {detail && (
          <p className="mt-1 break-words text-[12px] text-[var(--color-text-dimmer)]">
            {detail}
          </p>
        )}

        <div className="mt-5 flex justify-end gap-2">
          <button
            ref={cancelRef}
            onClick={onCancel}
            className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-4 py-2 text-[13px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white focus:outline-none focus:ring-2 focus:ring-[var(--color-accent)]/30"
          >
            {t("common.cancel")}
          </button>
          <button
            onClick={onConfirm}
            className="rounded-md bg-red-600 px-4 py-2 text-[13px] text-white transition-colors hover:bg-red-500 focus:outline-none focus:ring-2 focus:ring-red-400/40"
          >
            {confirmLabel}
          </button>
        </div>
      </div>
    </div>,
    document.body,
  );
}
