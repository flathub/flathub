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
} from "./playback-bar-layout";
import { usePlayerStore } from "@/stores/player-store";
import type { WindowShellTier } from "@/types/ipc";

interface PlaybackBarProps {
  densityOverride?: PlaybackBarDensity;
  shellTier?: WindowShellTier;
}

export function PlaybackBar({
  densityOverride,
  shellTier = "desktop",
}: PlaybackBarProps = {}) {
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
  const nativeVariant = shellTier === "mac_native";
  const layoutTokens = getPlaybackBarLayoutTokens(density, shellTier);
  const centerMinWidth = getPlaybackBarCenterMinWidth(density, shellTier);
  const shouldHideNowPlaying =
    !densityOverride && measuredWidth >= PLAYBACK_BAR_METADATA_COLLAPSE_WIDTH
      ? false
      : !densityOverride && shouldCollapsePlaybackBarMetadata(measuredWidth);

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
      className={`app-panel-surface flex shrink-0 flex-col justify-center ${layoutTokens.barHeightClass} ${
        nativeVariant
          ? "mx-3 mb-3 mt-2 rounded-[24px] border border-[var(--native-playback-border)] bg-[var(--native-playback-bg)] shadow-[var(--native-panel-shadow)]"
          : "border-t border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)] shadow-[0_-1px_0_rgba(255,255,255,0.02)]"
      }`}
      data-playback-bar-density={density}
      data-playback-bar-visual-variant={nativeVariant ? "native" : "default"}
      style={{ paddingInline: layoutTokens.outerPadding }}
    >
      <div className="grid w-full min-w-0 items-center" style={zoneStyle}>
        {!shouldHideNowPlaying && (
          <div
            data-playback-zone="left"
            className="min-w-0"
            style={{ maxWidth: layoutTokens.leftMaxWidth }}
          >
            <NowPlayingInfo density={density} shellTier={shellTier} />
          </div>
        )}

        <div
          data-playback-zone="center"
          className="grid min-w-0 items-center"
          style={centerZoneStyle}
        >
          <PlayControls density={density} shellTier={shellTier} />
          <SeekBar density={density} />
        </div>

        <div
          data-playback-zone="right"
          className="flex shrink-0 items-center justify-end"
          style={{ gap: layoutTokens.rightZoneGap }}
        >
          <QueueButton shellTier={shellTier} />
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
                className="motion-icon-button rounded-full p-1.5 text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
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
