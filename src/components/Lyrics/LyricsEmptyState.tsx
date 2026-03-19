import { useState } from "react";
import { useTranslation } from "react-i18next";
import { LyricsEditDialog } from "./LyricsEditDialog";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";

export function LyricsEmptyState() {
  const { t } = useTranslation();
  const [editOpen, setEditOpen] = useState(false);
  const songId = usePlayerStore((s) => s.snapshot?.song_id);
  const rawLrc = useLyricsStore((s) => s.rawLrc);

  return (
    <div className="flex flex-1 flex-col items-center justify-center gap-4">
      <p className="text-[14px] text-[var(--color-text-dimmer)]">
        {t("lyrics.noLyrics")}
      </p>
      {songId && (
        <>
          <button
            onClick={() => setEditOpen(true)}
            className="motion-surface rounded-md bg-[var(--color-hover)] px-4 py-2 text-[13px] text-[var(--color-control-primary)] hover:bg-[var(--color-active)]"
          >
            {t("lyrics.addLyrics")}
          </button>
          <LyricsEditDialog
            open={editOpen}
            onClose={() => setEditOpen(false)}
            songId={songId}
            existingLyrics={rawLrc || undefined}
          />
        </>
      )}
    </div>
  );
}
