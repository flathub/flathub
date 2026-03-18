import { useTranslation } from "react-i18next";
import { CoverArtThumbnail } from "@/components/Shared/CoverArtThumbnail";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";

export function NowPlayingInfo() {
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

  return (
    <div
      key={snapshot.song_id}
      className="flex items-center gap-3 overflow-hidden animate-[song-fade-in_var(--motion-duration-slow)_var(--motion-ease-emphasized-out)]"
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
        <span className="truncate text-[10px] text-[var(--color-text-dim)]">
          {artist}
        </span>
      </div>
    </div>
  );
}
