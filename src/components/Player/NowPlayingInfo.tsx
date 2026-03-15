import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";

export function NowPlayingInfo() {
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
    <div className="flex flex-col overflow-hidden">
      <span className="truncate text-[12px] font-medium text-white">
        {song?.title || "Unknown Title"}
      </span>
      <span className="truncate text-[10px] text-[var(--color-text-dim)]">
        {song?.artist || "Unknown Artist"}
      </span>
    </div>
  );
}
