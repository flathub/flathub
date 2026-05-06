import { type CSSProperties, useEffect, useRef, useState } from "react";
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
  getPlaybackBarCenterMinWidth,
  getPlaybackBarDensity,
  getPlaybackBarLayoutTokens,
  PLAYBACK_BAR_LEFT_MIN_WIDTH,
  PLAYBACK_BAR_METADATA_COLLAPSE_WIDTH,
  type PlaybackBarDensity,
  shouldCollapsePlaybackBarMetadata,
  shouldHideCoverArt,
} from "./playback-bar-layout";
import { usePlayerStore } from "@/stores/player-store";

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
  const [measuredWidth, setMeasuredWidth] = useState(1280);
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
      const width = Math.ceil(container.getBoundingClientRect().width);
      setMeasuredWidth((current) => (current === width ? current : width));
      const nextDensity = getPlaybackBarDensity(width);
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
  const layoutTokens = getPlaybackBarLayoutTokens(density);
  const centerMinWidth = getPlaybackBarCenterMinWidth(density);
  const shouldHideNowPlaying =
    !densityOverride && measuredWidth >= PLAYBACK_BAR_METADATA_COLLAPSE_WIDTH
      ? false
      : !densityOverride && shouldCollapsePlaybackBarMetadata(measuredWidth);
  const hideCoverArt = !densityOverride && shouldHideCoverArt(measuredWidth);

  const zoneStyle: CSSProperties = {
    gridTemplateColumns: shouldHideNowPlaying
      ? `minmax(${centerMinWidth}px, 1fr) max-content`
      : `minmax(${PLAYBACK_BAR_LEFT_MIN_WIDTH}px, ${layoutTokens.leftMaxWidth}px) minmax(${centerMinWidth}px, 1fr) max-content`,
    columnGap: layoutTokens.zoneGap,
  };
  const centerZoneStyle: CSSProperties = {
    gridTemplateColumns: `auto minmax(0, 1fr)`,
    columnGap: layoutTokens.zoneGap,
  };

  return (
    <div
      ref={containerRef}
      className={`app-panel-surface z-10 mx-3 mb-3 mt-2 flex shrink-0 flex-col justify-center rounded-[24px] border border-[var(--playback-bar-surface-border)] bg-[var(--playback-bar-surface-bg)] shadow-[var(--chrome-panel-shadow)] ${layoutTokens.barHeightClass}`}
      data-playback-bar-density={density}
      data-playback-bar-visual-variant="unified"
      style={{ paddingInline: layoutTokens.outerPadding }}
    >
      <div className="grid w-full min-w-0 items-center" style={zoneStyle}>
        {!shouldHideNowPlaying && (
          <div
            data-playback-zone="left"
            className="min-w-0"
            style={{ maxWidth: layoutTokens.leftMaxWidth }}
          >
            <NowPlayingInfo density={density} hideCoverArt={hideCoverArt} />
          </div>
        )}

        <div
          data-playback-zone="center"
          className="grid min-w-0 items-center"
          style={centerZoneStyle}
        >
          <PlayControls density={density} />
          <SeekBar density={density} />
        </div>

        <div
          data-playback-zone="right"
          className="flex shrink-0 items-center justify-end"
          style={{ gap: layoutTokens.rightZoneGap }}
        >
          <QueueButton />
          <VolumeSliders density={density} />

          <div
            className="flex shrink-0 items-center"
            style={{ gap: density === "relaxed" ? 8 : 6 }}
          >
            <Tooltip
              label={volume === 0 ? t("player.unmute") : t("player.mute")}
            >
              <button
                onClick={handleMasterMuteToggle}
                className="motion-icon-button min-h-11 min-w-11 rounded-full p-1.5 text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
                aria-label={
                  volume === 0 ? t("player.unmute") : t("player.mute")
                }
              >
                {volume === 0 ? <VolumeX size={14} /> : <Volume2 size={14} />}
              </button>
            </Tooltip>
            <AudioLevelSlider
              label={t("player.volume")}
              value={volume}
              onChange={setVolume}
              widthClass={layoutTokens.masterVolumeWidthClass}
              ariaLabel={t("player.volume")}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
