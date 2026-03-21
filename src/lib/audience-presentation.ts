import type { AirPlayColor, AudiencePresentationSpec } from "@/types/ipc";

const ACTIVE_TEXT_COLOR: AirPlayColor = {
  red: 1,
  green: 1,
  blue: 1,
  alpha: 1,
};

const PAST_TEXT_COLOR: AirPlayColor = {
  red: 72 / 255,
  green: 72 / 255,
  blue: 74 / 255,
  alpha: 1,
};

const FUTURE_TEXT_COLOR: AirPlayColor = {
  red: 58 / 255,
  green: 58 / 255,
  blue: 60 / 255,
  alpha: 1,
};

const PLAIN_TEXT_COLOR: AirPlayColor = {
  red: 1,
  green: 1,
  blue: 1,
  alpha: 1,
};

const STATUS_TEXT_COLOR: AirPlayColor = {
  red: 142 / 255,
  green: 142 / 255,
  blue: 147 / 255,
  alpha: 1,
};

const ACTIVE_GLOW_COLOR: AirPlayColor = {
  red: 1,
  green: 1,
  blue: 1,
  alpha: 0.8,
};

const AUDIENCE_FONT_SIZE_BY_STEP = {
  [-2]: 48,
  [-1]: 60,
  [0]: 72,
  [1]: 96,
  [2]: 96,
} as const;

function clampLyricsFontStep(step: number): -2 | -1 | 0 | 1 | 2 {
  return Math.max(-2, Math.min(2, step)) as -2 | -1 | 0 | 1 | 2;
}

// RATIONALE: The local fullscreen audience window and the AirPlay renderer
// must consume the same explicit spec or they will drift into two different
// products. Keep all cross-surface lyric layout and color decisions
// centralized here.
export function buildAudiencePresentationSpec(
  lyricsFontStep: number,
): AudiencePresentationSpec {
  return {
    contentWidthRatio: 0.92,
    contentMaxWidthPx: 1600,
    horizontalPaddingPx: 64,
    verticalPaddingPx: 56,
    lineGapPx: 40,
    fontSizePx: AUDIENCE_FONT_SIZE_BY_STEP[clampLyricsFontStep(lyricsFontStep)],
    lineHeightMultiple: 1.08,
    activeScale: 1.05,
    statusFontSizePx: 18,
    activeGlowBlurPx: 12,
    activeTextColor: ACTIVE_TEXT_COLOR,
    pastTextColor: PAST_TEXT_COLOR,
    futureTextColor: FUTURE_TEXT_COLOR,
    plainTextColor: PLAIN_TEXT_COLOR,
    statusTextColor: STATUS_TEXT_COLOR,
    activeGlowColor: ACTIVE_GLOW_COLOR,
  };
}

export function colorToCss(color: AirPlayColor): string {
  const red = Math.round(color.red * 255);
  const green = Math.round(color.green * 255);
  const blue = Math.round(color.blue * 255);
  return `rgba(${red}, ${green}, ${blue}, ${color.alpha})`;
}
