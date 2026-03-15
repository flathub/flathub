import { useRef, useEffect, useState } from "react";
import { Edit2 } from "lucide-react";
import { LyricLine } from "./LyricLine";
import { LyricsOffsetControl } from "./LyricsOffsetControl";
import { LyricsEmptyState } from "./LyricsEmptyState";
import { LyricsEditDialog } from "./LyricsEditDialog";
import { useLyricsStore } from "@/stores/lyrics-store";
import { usePlayerStore } from "@/stores/player-store";

export function LyricsPanel() {
  const lines = useLyricsStore((s) => s.lines);
  const activeLineIndex = useLyricsStore((s) => s.activeLineIndex);
  const offsetMs = useLyricsStore((s) => s.offsetMs);
  const isLoading = useLyricsStore((s) => s.isLoading);
  const rawLrc = useLyricsStore((s) => s.rawLrc);
  const songId = usePlayerStore((s) => s.snapshot?.song_id);
  const positionMs = usePlayerStore((s) => s.positionMs);
  const adjustedMs = positionMs - offsetMs;
  const containerRef = useRef<HTMLDivElement>(null);
  const [editOpen, setEditOpen] = useState(false);

  const isPlainText = lines.length > 0 && lines.every((l) => l.time_ms === 0);

  // Auto-scroll to active line (disabled for plain text)
  useEffect(() => {
    if (isPlainText) return;
    if (activeLineIndex < 0 || !containerRef.current) return;
    const lineEl = containerRef.current.children[activeLineIndex] as
      | HTMLElement
      | undefined;
    lineEl?.scrollIntoView({ behavior: "smooth", block: "center" });
  }, [activeLineIndex, isPlainText]);

  if (!songId) {
    return (
      <div className="flex flex-1 items-center justify-center">
        <p className="text-[14px] text-[var(--color-text-dimmer)]">
          Select a song to start
        </p>
      </div>
    );
  }

  if (isLoading) {
    return (
      <div className="flex flex-1 items-center justify-center">
        <p className="text-[14px] text-[var(--color-text-dim)]">
          Loading lyrics...
        </p>
      </div>
    );
  }

  if (lines.length === 0) {
    return <LyricsEmptyState />;
  }

  return (
    <div className="relative flex flex-1 flex-col items-center overflow-hidden">
      {songId && (
        <>
          <button
            onClick={() => setEditOpen(true)}
            className="absolute right-4 top-4 z-10 rounded-md p-1.5 text-[var(--color-text-dimmer)] transition-colors hover:bg-[#3A3A3C] hover:text-[#EBEBF5]"
            title="Edit lyrics"
          >
            <Edit2 size={14} />
          </button>
          <LyricsEditDialog
            open={editOpen}
            onClose={() => setEditOpen(false)}
            songId={songId}
            existingLyrics={rawLrc || undefined}
          />
        </>
      )}
      <div
        ref={containerRef}
        key={songId}
        className="custom-scrollbar flex w-full max-w-2xl flex-1 flex-col items-center gap-7 overflow-y-auto px-12 py-8 animate-[song-fade-in_300ms_ease-out]"
      >
        {lines.map((line, idx) => (
          <LyricLine
            key={idx}
            line={line}
            state={
              isPlainText
                ? "future"
                : idx === activeLineIndex
                  ? "active"
                  : idx < activeLineIndex
                    ? "past"
                    : "future"
            }
            adjustedMs={isPlainText ? 0 : adjustedMs}
          />
        ))}
      </div>
      <LyricsOffsetControl />
    </div>
  );
}
