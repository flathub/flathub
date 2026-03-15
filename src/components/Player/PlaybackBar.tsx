import { useRef } from "react";
import { Volume2, VolumeX } from "lucide-react";
import { NowPlayingInfo } from "./NowPlayingInfo";
import { PlayControls } from "./PlayControls";
import { SeekBar } from "./SeekBar";
import { VolumeSliders } from "./VolumeSliders";
import { QueueButton } from "./QueueButton";
import { usePlayerStore } from "@/stores/player-store";

export function PlaybackBar() {
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
    <div className="flex h-20 shrink-0 flex-col justify-center border-t border-[var(--color-border)] bg-[var(--color-toolbar)] px-4">
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
            className="text-[var(--color-text-dim)] transition-colors hover:text-white"
            title={volume === 0 ? "Unmute" : "Mute"}
            aria-label={volume === 0 ? "Unmute" : "Mute"}
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
            title="Volume"
            aria-label="Volume"
          />
        </div>
      </div>
    </div>
  );
}
