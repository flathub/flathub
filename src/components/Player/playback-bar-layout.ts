export type PlaybackBarDensity = "relaxed" | "compact" | "tight";

export const PLAYBACK_BAR_LEFT_MIN_WIDTH = 112;
export const PLAYBACK_BAR_SEEK_MIN_WIDTH = 248;
export const PLAYBACK_BAR_SEEK_MIN_WIDTH_CLASS = "min-w-[248px]";
export const PLAYBACK_BAR_SEEK_RAIL_MIN_WIDTH = 160;
export const PLAYBACK_BAR_SEEK_RAIL_MIN_WIDTH_CLASS = "min-w-[160px]";
export const PLAYBACK_BAR_TIME_LABEL_WIDTH_REM = 3.25;
export const PLAYBACK_BAR_TIME_LABEL_WIDTH_CLASS = "w-[3.25rem]";
export const PLAYBACK_BAR_CONTROL_CLUSTER_MIN_WIDTH = 120;
export const PLAYBACK_BAR_METADATA_COLLAPSE_WIDTH = 760;

export interface PlaybackBarLayoutTokens {
  leftMaxWidth: number;
  masterVolumeWidth: number;
  masterVolumeWidthClass: string;
  outerPadding: number;
  zoneGap: number;
  rightZoneGap: number;
}

const PLAYBACK_BAR_LAYOUT_TOKENS: Record<
  PlaybackBarDensity,
  PlaybackBarLayoutTokens
> = {
  // Keep the utility cluster stable by letting metadata surrender width first.
  relaxed: {
    leftMaxWidth: 240,
    masterVolumeWidth: 80,
    masterVolumeWidthClass: "w-20",
    outerPadding: 16,
    zoneGap: 16,
    rightZoneGap: 12,
  },
  compact: {
    leftMaxWidth: 200,
    masterVolumeWidth: 56,
    masterVolumeWidthClass: "w-14",
    outerPadding: 12,
    zoneGap: 12,
    rightZoneGap: 8,
  },
  tight: {
    leftMaxWidth: 160,
    masterVolumeWidth: 40,
    masterVolumeWidthClass: "w-10",
    outerPadding: 10,
    zoneGap: 8,
    rightZoneGap: 6,
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
