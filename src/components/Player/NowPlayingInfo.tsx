import { useTranslation } from "react-i18next";
import { CoverArtThumbnail } from "@/components/Shared/CoverArtThumbnail";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import type { PlaybackBarDensity } from "./playback-bar-layout";

interface NowPlayingInfoProps {
  density?: PlaybackBarDensity;
}

export function NowPlayingInfo({
  density = "relaxed",
}: NowPlayingInfoProps = {}) {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const songs = useLibraryStore((s) => s.songs);

  if (!snapshot?.song_id) {
    return (
      <div className="flex items-center gap-3">
        <CoverArtThumbnail
          songHash="now-playing-empty"
          coverArt={null}
          alt=""
          className="h-11 w-11 shrink-0"
        />
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
    >
      <CoverArtThumbnail
        songHash={snapshot.song_id}
        coverArt={song?.cover_art ?? null}
        alt={`${title} cover art`}
        className="h-11 w-11 shrink-0"
      />
      <div className="flex min-w-0 flex-col overflow-hidden">
        <span className="truncate text-[12px] font-medium text-white">
          {title}
        </span>
        {!hideArtist && (
          <span className="truncate text-[10px] text-[var(--color-text-dim)]">
            {artist}
          </span>
        )}
      </div>
    </div>
  );
}
