import { useRef } from "react";
import { useTranslation } from "react-i18next";
import { Volume2, VolumeX } from "lucide-react";
import { NowPlayingInfo } from "./NowPlayingInfo";
import { PlayControls } from "./PlayControls";
import { SeekBar } from "./SeekBar";
import { VolumeSliders } from "./VolumeSliders";
import { QueueButton } from "./QueueButton";
import { usePlayerStore } from "@/stores/player-store";

export function PlaybackBar() {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const setVolume = usePlayerStore((s) => s.setVolume);
  const volume = snapshot?.volume ?? 1;
  const prevVolumeRef = useRef(1);

  const handleMasterMuteToggle = () => {
    if (volume > 0) {
      prevVolumeRef.current = volume;
      setVolume(0);
    } else {
      setVolume(prevVolumeRef.current);
    }
  };

  return (
    <div className="flex h-20 shrink-0 flex-col justify-center border-t border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)] px-4 shadow-[0_-1px_0_rgba(255,255,255,0.02)] backdrop-blur-xl">
      <div className="flex w-full items-center gap-4">
        {/* Song info — fixed width left column */}
        <div className="w-[200px] shrink-0">
          <NowPlayingInfo />
        </div>

        {/* Play controls */}
        <PlayControls />

        {/* Seek bar — takes remaining space */}
        <SeekBar />

        {/* Queue button */}
        <QueueButton />

        {/* Stem volume sliders (visible when stems loaded) */}
        <VolumeSliders />

        {/* Master volume */}
        <div className="flex shrink-0 items-center gap-2">
          <button
            onClick={handleMasterMuteToggle}
            className="motion-icon-button rounded-full p-1.5 text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
            title={volume === 0 ? t("player.unmute") : t("player.mute")}
            aria-label={volume === 0 ? t("player.unmute") : t("player.mute")}
          >
            {volume === 0 ? <VolumeX size={14} /> : <Volume2 size={14} />}
          </button>
          <input
            type="range"
            min="0"
            max="100"
            value={Math.round(volume * 100)}
            onChange={(e) => setVolume(Number(e.target.value) / 100)}
            className="native-slider w-20"
            title={t("player.volume")}
            aria-label={t("player.volume")}
          />
        </div>
      </div>
    </div>
  );
}
