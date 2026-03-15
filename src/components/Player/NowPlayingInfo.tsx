import { useTranslation } from "react-i18next";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";

export function NowPlayingInfo() {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const songs = useLibraryStore((s) => s.songs);

  if (!snapshot?.song_id) {
    return (
      <span className="text-[12px] font-medium text-[var(--color-text-dim)]">
        OpenKara
      </span>
    );
  }

  const song = songs.find((s) => s.hash === snapshot.song_id);

  return (
    <div
      key={snapshot.song_id}
      className="flex flex-col overflow-hidden animate-[song-fade-in_300ms_ease-out]"
    >
      <span className="truncate text-[12px] font-medium text-white">
        {song?.title || t("common.unknownTitle")}
      </span>
      <span className="truncate text-[10px] text-[var(--color-text-dim)]">
        {song?.artist || t("common.unknownArtist")}
      </span>
    </div>
  );
}
