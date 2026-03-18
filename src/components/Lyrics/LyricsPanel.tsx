import { useRef, useEffect, useState } from "react";
import { useTranslation } from "react-i18next";
import { Edit2 } from "lucide-react";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { LyricLine } from "./LyricLine";
import { LyricsOffsetControl } from "./LyricsOffsetControl";
import { LyricsEmptyState } from "./LyricsEmptyState";
import { LyricsEditDialog } from "./LyricsEditDialog";
import { useLyricsStore } from "@/stores/lyrics-store";
import { usePlayerStore } from "@/stores/player-store";

interface LyricsPanelProps {
  presentation?: "standard" | "audience";
}

export function LyricsPanel({ presentation = "standard" }: LyricsPanelProps) {
  const { t } = useTranslation();
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
  const utilityControlsPinned = offsetMs !== 0;
  const isAudience = presentation === "audience";

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
          {t("lyrics.selectSong")}
        </p>
      </div>
    );
  }

  if (isLoading) {
    return (
      <div className="flex flex-1 items-center justify-center">
        <p className="text-[14px] text-[var(--color-text-dim)]">
          {t("lyrics.loadingLyrics")}
        </p>
      </div>
    );
  }

  if (lines.length === 0) {
    return <LyricsEmptyState />;
  }

  return (
    <div className="group relative flex flex-1 flex-col items-center overflow-hidden">
      {songId && !isAudience && (
        <>
          <div
            className="contextual-reveal absolute right-4 top-4 z-10"
            data-visible={utilityControlsPinned}
          >
            <Tooltip label={t("lyrics.editTooltip")}>
              <button
                onClick={() => setEditOpen(true)}
                aria-label={t("lyrics.editTooltip")}
                className="motion-icon-button rounded-full border border-[color-mix(in_srgb,var(--color-border-light)_78%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_76%,transparent)] p-2 text-[var(--color-text-dim)] shadow-[0_16px_30px_rgba(0,0,0,0.22)] backdrop-blur-xl hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_78%,transparent)] hover:text-[#EBEBF5] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35"
              >
                <Edit2 size={14} />
              </button>
            </Tooltip>
          </div>
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
        className={`custom-scrollbar flex w-full flex-1 overflow-y-auto animate-[song-fade-in_var(--motion-duration-slow)_var(--motion-ease-emphasized-out)] ${
          isAudience ? "px-12 py-10 md:px-16 md:py-14" : "px-12 py-8"
        }`}
      >
        <div
          className={`mx-auto flex w-full flex-col items-center ${
            isAudience
              ? "min-h-full max-w-[min(92vw,1600px)] justify-center gap-10"
              : "max-w-2xl gap-7"
          }`}
        >
          {lines.map((line, idx) => (
            <LyricLine
              key={idx}
              line={line}
              state={
                isPlainText
                  ? "plain"
                  : idx === activeLineIndex
                    ? "active"
                    : idx < activeLineIndex
                      ? "past"
                      : "future"
              }
              adjustedMs={isPlainText ? 0 : adjustedMs}
              presentation={presentation}
            />
          ))}
        </div>
      </div>
      {!isAudience && (
        <div
          className="pointer-events-none absolute inset-x-0 bottom-0 z-10 flex justify-center px-6 pb-5"
          data-visible={utilityControlsPinned}
        >
          <LyricsOffsetControl
            className="contextual-reveal pointer-events-auto"
            data-visible={utilityControlsPinned}
          />
        </div>
      )}
    </div>
  );
}
