import type { HTMLAttributes } from "react";
import { useLyricsStore } from "@/stores/lyrics-store";

type LyricsOffsetControlProps = HTMLAttributes<HTMLDivElement>;

export function LyricsOffsetControl({
  className = "",
  ...props
}: LyricsOffsetControlProps) {
  const songId = useLyricsStore((s) => s.songId);
  const offsetMs = useLyricsStore((s) => s.offsetMs);
  const adjustOffset = useLyricsStore((s) => s.adjustOffset);

  if (!songId) return null;

  return (
    <div
      className={`flex shrink-0 items-center gap-2 rounded-full border border-[var(--color-border-light)] bg-[color-mix(in_srgb,var(--color-sidebar)_80%,transparent)] px-2.5 py-2 text-[11px] text-[var(--color-text-dim)] shadow-[0_16px_34px_rgba(0,0,0,0.24)] backdrop-blur-xl ${className}`}
      {...props}
    >
      <button
        onClick={() => adjustOffset(songId, -500)}
        className="motion-surface rounded-full border border-[var(--color-border-light)] px-2.5 py-1 font-medium hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_72%,transparent)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35"
        aria-label="Adjust lyrics backward by 0.5 seconds"
      >
        -0.5s
      </button>
      <div className="min-w-[4.75rem] text-center">
        <div
          className={`font-[tabular-nums] text-[12px] font-medium ${
            offsetMs === 0 ? "text-[var(--color-text)]" : "text-white"
          }`}
        >
          {offsetMs >= 0 ? "+" : ""}
          {(offsetMs / 1000).toFixed(1)}s
        </div>
      </div>
      <button
        onClick={() => adjustOffset(songId, 500)}
        className="motion-surface rounded-full border border-[var(--color-border-light)] px-2.5 py-1 font-medium hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_72%,transparent)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35"
        aria-label="Adjust lyrics forward by 0.5 seconds"
      >
        +0.5s
      </button>
    </div>
  );
}
