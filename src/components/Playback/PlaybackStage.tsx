import { LyricsPanel } from "@/components/Lyrics/LyricsPanel";
import { CdgCanvas } from "@/components/Cdg/CdgCanvas";
import { useCdgStore } from "@/stores/cdg-store";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { songHasCdgMedia } from "@/lib/song-media";
import { getShortcutPlatform } from "@/lib/app-shortcuts";

interface PlaybackStageProps {
  presentation?: "standard" | "audience";
  bottomInsetPx?: number;
}

export function PlaybackStage({
  presentation = "standard",
  bottomInsetPx = 0,
}: PlaybackStageProps) {
  const hasCdg = useCdgStore((s) => s.hasCdg);
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);
  const songs = useLibraryStore((s) => s.songs);
  const currentSongHasCdg = songHasCdgMedia(
    songs.find((song) => song.hash === songId) ?? null,
  );

  const macStandardChrome =
    presentation === "standard" && getShortcutPlatform() === "mac";

  return (
    <div
      className={`flex h-full w-full flex-1 overflow-hidden ${macStandardChrome ? "pointer-events-none" : ""}`}
      style={
        presentation === "audience" && bottomInsetPx > 0
          ? { paddingBottom: bottomInsetPx }
          : undefined
      }
    >
      {hasCdg || currentSongHasCdg ? (
        <div
          className={
            macStandardChrome
              ? "pointer-events-auto flex min-h-0 w-full flex-1 flex-col"
              : "contents"
          }
        >
          <CdgCanvas />
        </div>
      ) : (
        <LyricsPanel
          presentation={presentation}
          pointerEventsCoexistWithDragRegion={macStandardChrome}
        />
      )}
    </div>
  );
}
