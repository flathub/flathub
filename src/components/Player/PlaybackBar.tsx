import { useEffect, useRef, useState } from "react";
import { useTranslation } from "react-i18next";
import { Volume2, VolumeX } from "lucide-react";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { NowPlayingInfo } from "./NowPlayingInfo";
import { PlayControls } from "./PlayControls";
import { SeekBar } from "./SeekBar";
import { VolumeSliders } from "./VolumeSliders";
import { QueueButton } from "./QueueButton";
import { AudioLevelSlider } from "./AudioLevelSlider";
import {
  getPlaybackBarDensity,
  type PlaybackBarDensity,
} from "./playback-bar-layout";
import { usePlayerStore } from "@/stores/player-store";

const INNER_GAP_BY_DENSITY: Record<PlaybackBarDensity, string> = {
  relaxed: "gap-4",
  compact: "gap-3",
  tight: "gap-2",
};

const OUTER_PADDING_BY_DENSITY: Record<PlaybackBarDensity, string> = {
  relaxed: "px-4",
  compact: "px-3",
  tight: "px-2.5",
};

const NOW_PLAYING_WIDTH_BY_DENSITY: Record<PlaybackBarDensity, string> = {
  relaxed: "w-[240px]",
  compact: "w-[200px]",
  tight: "w-[160px]",
};

const MASTER_VOLUME_WIDTH_BY_DENSITY: Record<PlaybackBarDensity, string> = {
  relaxed: "w-20",
  compact: "w-16",
  tight: "w-12",
};

interface PlaybackBarProps {
  densityOverride?: PlaybackBarDensity;
}

export function PlaybackBar({ densityOverride }: PlaybackBarProps = {}) {
  const { t } = useTranslation();
  const snapshot = usePlayerStore((s) => s.snapshot);
  const setVolume = usePlayerStore((s) => s.setVolume);
  const volume = snapshot?.volume ?? 1;
  const prevVolumeRef = useRef(1);
  const containerRef = useRef<HTMLDivElement>(null);
  const [measuredDensity, setMeasuredDensity] =
    useState<PlaybackBarDensity>("relaxed");

  useEffect(() => {
    if (densityOverride) {
      return;
    }

    const container = containerRef.current;
    if (!container) {
      return;
    }

    const measure = () => {
      const nextDensity = getPlaybackBarDensity(
        Math.ceil(container.getBoundingClientRect().width),
      );
      setMeasuredDensity((current) =>
        current === nextDensity ? current : nextDensity,
      );
    };

    measure();

    if (typeof ResizeObserver !== "undefined") {
      const observer = new ResizeObserver(measure);
      observer.observe(container);
      return () => observer.disconnect();
    }

    window.addEventListener("resize", measure);
    return () => {
      window.removeEventListener("resize", measure);
    };
  }, [densityOverride]);

  const handleMasterMuteToggle = () => {
    if (volume > 0) {
      prevVolumeRef.current = volume;
      setVolume(0);
    } else {
      setVolume(prevVolumeRef.current);
    }
  };

  const density = densityOverride ?? measuredDensity;

  return (
    <div
      ref={containerRef}
      className={`app-panel-surface flex h-20 shrink-0 flex-col justify-center border-t border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)] shadow-[0_-1px_0_rgba(255,255,255,0.02)] ${OUTER_PADDING_BY_DENSITY[density]}`}
      data-playback-bar-density={density}
    >
      <div
        className={`flex w-full items-center ${INNER_GAP_BY_DENSITY[density]}`}
      >
        {/* Song info — fixed width left column */}
        <div
          className={`min-w-0 shrink-0 ${NOW_PLAYING_WIDTH_BY_DENSITY[density]}`}
        >
          <NowPlayingInfo density={density} />
        </div>

        {/* Play controls */}
        <PlayControls density={density} />

        {/* Seek bar — takes remaining space */}
        <SeekBar density={density} />

        {/* Queue button */}
        <QueueButton />

        {/* Stem volume sliders (visible when stems loaded) */}
        <VolumeSliders density={density} />

        {/* Master volume */}
        <div
          className={`flex shrink-0 items-center ${density === "relaxed" ? "gap-2" : "gap-1.5"}`}
        >
          <Tooltip label={volume === 0 ? t("player.unmute") : t("player.mute")}>
            <button
              onClick={handleMasterMuteToggle}
              className="motion-icon-button rounded-full p-1.5 text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
              aria-label={volume === 0 ? t("player.unmute") : t("player.mute")}
            >
              {volume === 0 ? <VolumeX size={14} /> : <Volume2 size={14} />}
            </button>
          </Tooltip>
          <AudioLevelSlider
            label={t("player.volume")}
            value={volume}
            onChange={setVolume}
            widthClass={MASTER_VOLUME_WIDTH_BY_DENSITY[density]}
            ariaLabel={t("player.volume")}
          />
        </div>
      </div>
    </div>
  );
}
