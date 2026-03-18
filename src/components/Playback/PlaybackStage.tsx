import { LyricsPanel } from "@/components/Lyrics/LyricsPanel";
import { CdgCanvas } from "@/components/Cdg/CdgCanvas";
import { useCdgStore } from "@/stores/cdg-store";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { songHasCdgMedia } from "@/lib/song-media";

export function PlaybackStage() {
  const hasCdg = useCdgStore((s) => s.hasCdg);
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);
  const songs = useLibraryStore((s) => s.songs);
  const currentSongHasCdg = songHasCdgMedia(
    songs.find((song) => song.hash === songId) ?? null,
  );

  return hasCdg || currentSongHasCdg ? <CdgCanvas /> : <LyricsPanel />;
}
