import { Play, Pause, SkipBack, SkipForward } from "lucide-react";
import { useTranslation } from "react-i18next";
import { usePlayerStore } from "@/stores/player-store";

export function PlayControls() {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const resume = usePlayerStore((s) => s.resume);
  const pause = usePlayerStore((s) => s.pause);
  const skipBack = usePlayerStore((s) => s.skipBack);
  const skipForward = usePlayerStore((s) => s.skipForward);
  const isPlaying = snapshot?.is_playing ?? false;

  const handleToggle = () => {
    if (isPlaying) {
      pause();
    } else if (snapshot?.song_id) {
      resume();
    }
  };

  return (
    <div className="flex items-center gap-4 text-[#EBEBF5]">
      <button
        onClick={skipBack}
        className="motion-icon-button rounded-full p-1.5 opacity-80 hover:bg-white/4 hover:text-white hover:opacity-100 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
        title={t("player.previous")}
        aria-label={t("player.previous")}
      >
        <SkipBack size={20} fill="currentColor" />
      </button>
      <button
        onClick={handleToggle}
        className="motion-icon-button flex h-8 w-8 items-center justify-center rounded-full bg-[#EBEBF5] text-[var(--color-surface)] shadow-[0_10px_24px_rgba(0,0,0,0.22)] hover:bg-white hover:shadow-[0_14px_28px_rgba(0,0,0,0.28)] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-white/30"
        title={isPlaying ? t("player.pause") : t("player.play")}
        aria-label={isPlaying ? t("player.pause") : t("player.play")}
      >
        {isPlaying ? (
          <Pause size={16} fill="currentColor" />
        ) : (
          <Play size={16} fill="currentColor" className="ml-0.5" />
        )}
      </button>
      <button
        onClick={skipForward}
        className="motion-icon-button rounded-full p-1.5 opacity-80 hover:bg-white/4 hover:text-white hover:opacity-100 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
        title={t("player.next")}
        aria-label={t("player.next")}
      >
        <SkipForward size={20} fill="currentColor" />
      </button>
    </div>
  );
}
