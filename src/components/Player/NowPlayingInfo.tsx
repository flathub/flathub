import { useEffect, useRef, useState } from "react";
import { useTranslation } from "react-i18next";
import { CoverArtThumbnail } from "@/components/Shared/CoverArtThumbnail";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import type { PlaybackBarDensity } from "./playback-bar-layout";

interface NowPlayingInfoProps {
  density?: PlaybackBarDensity;
  hideCoverArt?: boolean;
}

export function NowPlayingInfo({
  density = "relaxed",
  hideCoverArt = false,
}: NowPlayingInfoProps = {}) {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const songs = useLibraryStore((s) => s.songs);

  if (!snapshot?.song_id) {
    return (
      <div className="flex items-center gap-3">
        <span className="truncate text-[12px] font-medium text-[var(--color-text-dim)]">
          OpenKara
        </span>
      </div>
    );
  }

  const song = songs.find((s) => s.hash === snapshot.song_id);
  const title = song?.title || t("common.unknownTitle");
  const artist = song?.artist || t("common.unknownArtist");
  const hideArtist = density === "tight";
  const hasCoverArt =
    song?.cover_art != null &&
    (() => {
      const ca = song.cover_art;
      if (ca instanceof ArrayBuffer) return ca.byteLength > 0;
      if (Array.isArray(ca)) return ca.length > 0;
      return (ca as Uint8Array).length > 0;
    })();
  const showCoverArt = !hideCoverArt && hasCoverArt;

  return (
    <div
      key={snapshot.song_id}
      className={`flex items-center overflow-hidden animate-[song-fade-in_var(--motion-duration-slow)_var(--motion-ease-emphasized-out)] ${
        density === "relaxed"
          ? "gap-3"
          : density === "compact"
            ? "gap-2.5"
            : "gap-2"
      }`}
      data-now-playing-visual-variant="unified"
    >
      {showCoverArt && (
        <CoverArtThumbnail
          songHash={snapshot.song_id}
          coverArt={song!.cover_art!}
          alt={`${title} cover art`}
          className="h-12 w-12 shrink-0"
        />
      )}
      <div className="flex min-w-0 flex-col overflow-hidden">
        <MarqueeTitle title={title} />
        {!hideArtist && (
          <span className="truncate text-[12px] text-[var(--color-text-dim)]">
            {artist}
          </span>
        )}
      </div>
    </div>
  );
}

function MarqueeTitle({ title }: { title: string }) {
  const containerRef = useRef<HTMLSpanElement>(null);
  const textRef = useRef<HTMLSpanElement>(null);
  const [overflowing, setOverflowing] = useState(false);
  const [marqueeOffset, setMarqueeOffset] = useState("0px");

  useEffect(() => {
    const container = containerRef.current;
    const text = textRef.current;
    if (!container || !text) return;

    const measure = () => {
      const overflows = text.scrollWidth > container.clientWidth;
      setOverflowing(overflows);
      if (overflows) {
        const offset = container.clientWidth - text.scrollWidth;
        setMarqueeOffset(`${offset}px`);
      }
    };

    measure();

    if (typeof ResizeObserver !== "undefined") {
      const observer = new ResizeObserver(measure);
      observer.observe(container);
      return () => observer.disconnect();
    }
  }, [title]);

  return (
    <span
      ref={containerRef}
      className="block overflow-hidden text-[14px] font-semibold text-white"
    >
      <span
        ref={textRef}
        className={`inline-block whitespace-nowrap ${
          overflowing ? "marquee-active" : ""
        }`}
        style={
          overflowing
            ? ({ "--marquee-offset": marqueeOffset } as React.CSSProperties)
            : undefined
        }
      >
        {title}
      </span>
    </span>
  );
}
