import { useState } from "react";
import { LyricsEditDialog } from "./LyricsEditDialog";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";

export function LyricsEmptyState() {
  const [editOpen, setEditOpen] = useState(false);
  const songId = usePlayerStore((s) => s.snapshot?.song_id);
  const rawLrc = useLyricsStore((s) => s.rawLrc);

  return (
    <div className="flex flex-1 flex-col items-center justify-center gap-4">
      <p className="text-[14px] text-[var(--color-text-dimmer)]">
        No lyrics available for this track
      </p>
      {songId && (
        <>
          <button
            onClick={() => setEditOpen(true)}
            className="rounded-md bg-[#3A3A3C] px-4 py-2 text-[13px] text-[#EBEBF5] transition-colors hover:bg-[#48484A]"
          >
            Add Lyrics
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
