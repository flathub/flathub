import { useTranslation } from "react-i18next";
import { CoverArtThumbnail } from "@/components/Shared/CoverArtThumbnail";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import type { PlaybackBarDensity } from "./playback-bar-layout";
import type { WindowShellTier } from "@/types/ipc";

interface NowPlayingInfoProps {
  density?: PlaybackBarDensity;
  shellTier?: WindowShellTier;
}

export function NowPlayingInfo({
  density = "relaxed",
  shellTier = "desktop",
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
  const nativeVariant = shellTier === "mac_native";

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
      data-now-playing-visual-variant={nativeVariant ? "native" : "default"}
    >
      <CoverArtThumbnail
        songHash={snapshot.song_id}
        coverArt={song?.cover_art ?? null}
        alt={`${title} cover art`}
        className={`${nativeVariant ? "h-12 w-12" : "h-11 w-11"} shrink-0`}
      />
      <div className="flex min-w-0 flex-col overflow-hidden">
        <span
          className={`truncate text-white ${nativeVariant ? "text-[14px] font-semibold" : "text-[12px] font-medium"}`}
        >
          {title}
        </span>
        {!hideArtist && (
          <span
            className={`truncate text-[var(--color-text-dim)] ${nativeVariant ? "text-[12px]" : "text-[10px]"}`}
          >
            {artist}
          </span>
        )}
      </div>
    </div>
  );
}
