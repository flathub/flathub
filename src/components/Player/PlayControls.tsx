import { Play, Pause, SkipBack, SkipForward } from "lucide-react";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { usePlayerStore } from "@/stores/player-store";
import type { PlaybackBarDensity } from "./playback-bar-layout";
import type { WindowShellTier } from "@/types/ipc";

interface PlayControlsProps {
  density?: PlaybackBarDensity;
  shellTier?: WindowShellTier;
}

export function PlayControls({
  density = "relaxed",
  shellTier = "desktop",
}: PlayControlsProps = {}) {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const resume = usePlayerStore((s) => s.resume);
  const pause = usePlayerStore((s) => s.pause);
  const skipBack = usePlayerStore((s) => s.skipBack);
  const skipForward = usePlayerStore((s) => s.skipForward);
  const isPlaying = snapshot?.is_playing ?? false;
  const nativeVariant = shellTier === "mac_native";

  const handleToggle = () => {
    if (isPlaying) {
      pause();
    } else if (snapshot?.song_id) {
      resume();
    }
  };

  return (
    <div
      className={`flex items-center text-[var(--color-control-primary)] ${
        density === "relaxed"
          ? "gap-4"
          : density === "compact"
            ? "gap-2.5"
            : "gap-2"
      }`}
      data-play-controls-visual-variant={nativeVariant ? "native" : "default"}
    >
      <Tooltip label={t("player.previous")}>
        <button
          onClick={skipBack}
          className={`motion-icon-button rounded-full opacity-80 hover:bg-[var(--color-ghost-hover)] hover:text-white hover:opacity-100 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${nativeVariant ? "p-2" : "p-1.5"}`}
          aria-label={t("player.previous")}
        >
          <SkipBack size={20} fill="currentColor" />
        </button>
      </Tooltip>
      <Tooltip label={isPlaying ? t("player.pause") : t("player.play")}>
        <button
          onClick={handleToggle}
          className={`motion-icon-button flex items-center justify-center rounded-full bg-[var(--color-control-primary)] text-[var(--color-control-primary-foreground)] shadow-[0_10px_24px_rgba(0,0,0,0.22)] hover:bg-[color-mix(in_srgb,var(--color-control-primary)_90%,white)] hover:shadow-[0_14px_28px_rgba(0,0,0,0.28)] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-control-primary)]/30 ${nativeVariant ? "h-10 w-10" : "h-8 w-8"}`}
          aria-label={isPlaying ? t("player.pause") : t("player.play")}
        >
          {isPlaying ? (
            <Pause size={16} fill="currentColor" />
          ) : (
            <Play size={16} fill="currentColor" className="ml-0.5" />
          )}
        </button>
      </Tooltip>
      <Tooltip label={t("player.next")}>
        <button
          onClick={skipForward}
          className={`motion-icon-button rounded-full opacity-80 hover:bg-[var(--color-ghost-hover)] hover:text-white hover:opacity-100 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${nativeVariant ? "p-2" : "p-1.5"}`}
          aria-label={t("player.next")}
        >
          <SkipForward size={20} fill="currentColor" />
        </button>
      </Tooltip>
    </div>
  );
}
