import { useState } from "react";
import { useTranslation } from "react-i18next";
import { useLyricsStore } from "@/stores/lyrics-store";

interface LyricsEditDialogProps {
  open: boolean;
  onClose: () => void;
  songId: string;
  existingLyrics?: string;
}

export function LyricsEditDialog({
  open,
  onClose,
  songId,
  existingLyrics,
}: LyricsEditDialogProps) {
  if (!open) return null;

  return (
    <LyricsEditDialogContent
      key={songId}
      onClose={onClose}
      songId={songId}
      existingLyrics={existingLyrics}
    />
  );
}

function LyricsEditDialogContent({
  onClose,
  songId,
  existingLyrics,
}: Omit<LyricsEditDialogProps, "open">) {
  const { t } = useTranslation();
  const [text, setText] = useState(existingLyrics ?? "");
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const headingId = `lyrics-edit-heading-${songId}`;
  const textareaId = `lyrics-edit-text-${songId}`;
  const errorId = `lyrics-edit-error-${songId}`;

  const isLrc = /\[\d{2}:\d{2}/.test(text);

  const handleSave = async () => {
    if (saving) return;

    setSaveError(null);
    setSaving(true);
    const saved = await useLyricsStore
      .getState()
      .saveManualLyrics(songId, text);

    if (saved) {
      onClose();
      return;
    }

    setSaveError(t("errors.somethingWentWrong"));
    setSaving(false);
  };

  return (
    <div
      role="presentation"
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/60"
      onClick={(e) => {
        if (!saving && e.target === e.currentTarget) onClose();
      }}
    >
      <div
        role="dialog"
        aria-modal="true"
        aria-labelledby={headingId}
        aria-busy={saving}
        className="flex w-full max-w-lg flex-col gap-4 overflow-hidden rounded-xl border border-[#3A3A3C] bg-[#1C1C1E] p-6 shadow-2xl"
      >
        <h2 id={headingId} className="text-[15px] font-semibold text-white">
          {t("lyrics.editLyrics")}
        </h2>

        <label htmlFor={textareaId} className="sr-only">
          {t("lyrics.editLyrics")}
        </label>
        <textarea
          id={textareaId}
          value={text}
          onChange={(e) => setText(e.target.value)}
          placeholder={t("lyrics.pastePlaceholder")}
          className="h-64 w-full resize-y rounded-md border border-[#3A3A3C] bg-[#2C2C2E] px-3 py-2 text-[13px] text-white placeholder-[#636366] transition-colors focus:border-[#48484A] focus:outline-none focus:ring-2 focus:ring-[var(--color-accent)]/30"
          spellCheck={false}
        />

        <p className="text-[11px] text-[#8E8E93]">
          {text.trim().length > 0
            ? isLrc
              ? t("lyrics.detectedLrc")
              : t("lyrics.detectedPlain")
            : t("lyrics.supportsFormats")}
        </p>

        {saveError && (
          <p
            id={errorId}
            role="alert"
            className="break-words rounded-md border border-red-500/30 bg-red-500/10 px-3 py-2 text-[12px] text-red-300"
          >
            {saveError}
          </p>
        )}

        <div className="flex justify-end gap-2">
          <button
            onClick={onClose}
            disabled={saving}
            className="rounded-md px-4 py-2 text-[13px] text-[#EBEBF5] transition-colors hover:bg-[#3A3A3C] focus:outline-none focus:ring-2 focus:ring-[var(--color-accent)]/30 disabled:cursor-not-allowed disabled:opacity-50"
          >
            {t("common.cancel")}
          </button>
          <button
            onClick={handleSave}
            disabled={saving || text.trim().length === 0}
            aria-describedby={saveError ? errorId : undefined}
            className="rounded-md bg-[#3A3A3C] px-4 py-2 text-[13px] text-[#EBEBF5] transition-colors hover:bg-[#48484A] focus:outline-none focus:ring-2 focus:ring-[var(--color-accent)]/30 disabled:cursor-not-allowed disabled:opacity-40 disabled:hover:bg-[#3A3A3C]"
          >
            {saving ? t("common.saving") : t("common.save")}
          </button>
        </div>
      </div>
    </div>
  );
}
