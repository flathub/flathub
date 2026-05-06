export type PlaybackBarDensity = "relaxed" | "compact" | "tight";

export const PLAYBACK_BAR_LEFT_MIN_WIDTH = 112;
export const PLAYBACK_BAR_SEEK_MIN_WIDTH = 180;
export const PLAYBACK_BAR_SEEK_MIN_WIDTH_CLASS = "min-w-[180px]";
export const PLAYBACK_BAR_SEEK_RAIL_MIN_WIDTH = 120;
export const PLAYBACK_BAR_SEEK_RAIL_MIN_WIDTH_CLASS = "min-w-[120px]";
export const PLAYBACK_BAR_TIME_LABEL_WIDTH_REM = 3.25;
export const PLAYBACK_BAR_TIME_LABEL_WIDTH_CLASS = "w-[3.25rem]";
export const PLAYBACK_BAR_CONTROL_CLUSTER_MIN_WIDTH = 120;
export const PLAYBACK_BAR_METADATA_COLLAPSE_WIDTH = 760;
export const PLAYBACK_BAR_COVER_ART_COLLAPSE_WIDTH = 780;

export interface PlaybackBarLayoutTokens {
  leftMaxWidth: number;
  masterVolumeWidth: number;
  masterVolumeWidthClass: string;
  outerPadding: number;
  zoneGap: number;
  rightZoneGap: number;
  barHeightClass: string;
}

/** Single layout table shared across shells (formerly the mac-only tier table). */
const PLAYBACK_BAR_LAYOUT_TOKENS: Record<
  PlaybackBarDensity,
  PlaybackBarLayoutTokens
> = {
  relaxed: {
    leftMaxWidth: 268,
    masterVolumeWidth: 72,
    masterVolumeWidthClass: "w-[4.5rem]",
    outerPadding: 18,
    zoneGap: 18,
    rightZoneGap: 10,
    barHeightClass: "h-[86px]",
  },
  compact: {
    leftMaxWidth: 218,
    masterVolumeWidth: 52,
    masterVolumeWidthClass: "w-[3.25rem]",
    outerPadding: 14,
    zoneGap: 14,
    rightZoneGap: 8,
    barHeightClass: "h-[78px]",
  },
  tight: {
    leftMaxWidth: 160,
    masterVolumeWidth: 40,
    masterVolumeWidthClass: "w-10",
    outerPadding: 12,
    zoneGap: 10,
    rightZoneGap: 6,
    barHeightClass: "h-[70px]",
  },
};

export function getPlaybackBarDensity(width: number): PlaybackBarDensity {
  if (width < 960) {
    return "tight";
  }

  if (width < 1120) {
    return "compact";
  }

  return "relaxed";
}

export function getPlaybackBarLayoutTokens(
  density: PlaybackBarDensity,
): PlaybackBarLayoutTokens {
  return PLAYBACK_BAR_LAYOUT_TOKENS[density];
}

export function getPlaybackBarCenterMinWidth(
  density: PlaybackBarDensity,
): number {
  const { zoneGap } = getPlaybackBarLayoutTokens(density);

  return (
    PLAYBACK_BAR_CONTROL_CLUSTER_MIN_WIDTH +
    PLAYBACK_BAR_SEEK_MIN_WIDTH +
    zoneGap
  );
}

export function shouldCollapsePlaybackBarMetadata(width: number): boolean {
  return width < PLAYBACK_BAR_METADATA_COLLAPSE_WIDTH;
}

export function shouldHideCoverArt(width: number): boolean {
  return width < PLAYBACK_BAR_COVER_ART_COLLAPSE_WIDTH;
}
