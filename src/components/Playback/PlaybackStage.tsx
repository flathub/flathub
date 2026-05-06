import { LyricsPanel } from "@/components/Lyrics/LyricsPanel";
import { CdgCanvas } from "@/components/Cdg/CdgCanvas";
import { useCoverArtUrl } from "@/lib/cover-art";
import { useCdgStore } from "@/stores/cdg-store";
import { useLibraryStore } from "@/stores/library-store";
import { usePlayerStore } from "@/stores/player-store";
import { useSettingsStore } from "@/stores/settings-store";
import { songHasCdgMedia } from "@/lib/song-media";

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
  const currentSong = songs.find((song) => song.hash === songId) ?? null;
  const currentSongHasCdg = songHasCdgMedia(currentSong);
  const coverArtBackdrop = useSettingsStore((s) => s.coverArtBackdrop);
  const nativeBackdropUrl = useCoverArtUrl(
    songId ?? "native-stage-empty",
    currentSong?.cover_art ?? null,
  );
  const stageAmbience =
    coverArtBackdrop &&
    presentation === "standard" &&
    !hasCdg &&
    !currentSongHasCdg;

  return (
    <div
      className="relative flex h-full w-full flex-1 overflow-hidden"
      data-stage-visual-variant={stageAmbience ? "ambience" : "default"}
      style={
        presentation === "audience" && bottomInsetPx > 0
          ? { paddingBottom: bottomInsetPx }
          : undefined
      }
    >
      {stageAmbience ? (
        <>
          <div className="absolute inset-0" data-native-stage-backdrop="true">
            <div
              className="absolute inset-[-6%] scale-[1.06] bg-center bg-cover opacity-34 blur-2xl saturate-[0.92]"
              style={
                nativeBackdropUrl
                  ? { backgroundImage: `url(${nativeBackdropUrl})` }
                  : undefined
              }
            />
            <div className="absolute inset-0 bg-[radial-gradient(circle_at_center,rgba(255,255,255,0.04),rgba(0,0,0,0.08)_36%,rgba(0,0,0,0.48)_100%)]" />
            <div className="absolute inset-0 bg-[linear-gradient(180deg,rgba(12,14,18,0.22),rgba(11,13,16,0.54)_58%,rgba(13,15,18,0.72))]" />
          </div>
          <div className="relative z-10 flex min-h-0 flex-1 overflow-hidden">
            <LyricsPanel presentation={presentation} />
          </div>
        </>
      ) : hasCdg || currentSongHasCdg ? (
        <CdgCanvas />
      ) : (
        <LyricsPanel presentation={presentation} />
      )}
    </div>
  );
}
