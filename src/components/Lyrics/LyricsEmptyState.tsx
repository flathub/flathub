import { useState } from "react";
import { useTranslation } from "react-i18next";
import { LyricsEditDialog } from "./LyricsEditDialog";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";

interface LyricsEmptyStateProps {
  presentation?: "standard" | "audience";
}

export function LyricsEmptyState({
  presentation = "standard",
}: LyricsEmptyStateProps) {
  const { t } = useTranslation();
  const [editOpen, setEditOpen] = useState(false);
  const songId = usePlayerStore((s) => s.snapshot?.song_id);
  const rawLrc = useLyricsStore((s) => s.rawLrc);
  const isAudience = presentation === "audience";

  return (
    <div className="flex flex-1 flex-col items-center justify-center gap-4">
      <p className="text-[14px] text-[var(--color-text-dimmer)]">
        {t("lyrics.noLyrics")}
      </p>
      {songId && !isAudience && (
        <>
          {/* RATIONALE: The audience surface is passive output, not an editing
              surface. Reintroducing the add-lyrics CTA here would recreate the
              oversized background-info window we are intentionally removing. */}
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
