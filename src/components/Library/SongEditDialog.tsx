import { useState, useEffect, useRef } from "react";
import { useTranslation } from "react-i18next";
import { useLibraryStore } from "@/stores/library-store";
import type { Song } from "@/types/ipc";

interface SongEditDialogProps {
  song: Song;
  onClose: () => void;
}

export function SongEditDialog({ song, onClose }: SongEditDialogProps) {
  const { t } = useTranslation();
  const [title, setTitle] = useState(song.title ?? "");
  const [artist, setArtist] = useState(song.artist ?? "");
  const [saving, setSaving] = useState(false);
  const updateSongMetadata = useLibraryStore((s) => s.updateSongMetadata);
  const backdropRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", handleEscape);
    return () => window.removeEventListener("keydown", handleEscape);
  }, [onClose]);

  const handleBackdropClick = (e: React.MouseEvent) => {
    if (e.target === backdropRef.current) onClose();
  };

  const handleSave = async () => {
    setSaving(true);
    await updateSongMetadata(
      song.hash,
      title.trim() || null,
      artist.trim() || null,
    );
    setSaving(false);
    onClose();
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "Enter" && !saving) {
      e.preventDefault();
      handleSave();
    }
  };

  return (
    <div
      ref={backdropRef}
      onClick={handleBackdropClick}
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/50"
    >
      <div className="w-full max-w-sm rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] shadow-2xl">
        <div className="border-b border-[var(--color-border)] px-5 py-3">
          <h3 className="text-[14px] font-semibold text-white">{t("songEdit.title")}</h3>
        </div>
        <div className="space-y-4 p-5">
          <div className="space-y-1.5">
            <label className="text-[12px] font-medium text-[var(--color-text-dim)]">
              {t("songEdit.titleLabel")}
            </label>
            <input
              type="text"
              value={title}
              onChange={(e) => setTitle(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder={t("songEdit.titlePlaceholder")}
              autoFocus
              className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[13px] text-white placeholder:text-[var(--color-text-dimmer)] focus:border-[var(--color-accent)] focus:outline-none"
            />
          </div>
          <div className="space-y-1.5">
            <label className="text-[12px] font-medium text-[var(--color-text-dim)]">
              {t("songEdit.artistLabel")}
            </label>
            <input
              type="text"
              value={artist}
              onChange={(e) => setArtist(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder={t("songEdit.artistPlaceholder")}
              className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[13px] text-white placeholder:text-[var(--color-text-dimmer)] focus:border-[var(--color-accent)] focus:outline-none"
            />
          </div>
        </div>
        <div className="flex justify-end gap-2 border-t border-[var(--color-border)] px-5 py-3">
          <button
            onClick={onClose}
            className="rounded-md px-3 py-1.5 text-[12px] text-[var(--color-text-dim)] transition-colors hover:text-white"
          >
            {t("common.cancel")}
          </button>
          <button
            onClick={handleSave}
            disabled={saving}
            className="rounded-md bg-[var(--color-accent)] px-3 py-1.5 text-[12px] font-medium text-white transition-colors hover:bg-[var(--color-accent)]/80 disabled:opacity-50"
          >
            {saving ? t("common.saving") : t("common.save")}
          </button>
        </div>
      </div>
    </div>
  );
}
