import { memo } from "react";
import {
  buildAudiencePresentationSpec,
  colorToCss,
} from "@/lib/audience-presentation";
import { usePlayerStore } from "@/stores/player-store";
import type { LyricLine as LyricLineType, WordToken } from "@/types/ipc";

interface LyricLineProps {
  line: LyricLineType;
  state: "active" | "past" | "future" | "plain";
  adjustedMs: number;
  presentation?: "standard" | "audience";
  lyricsFontStep: number;
  romanizedText?: string;
}

const STANDARD_TEXT_SIZE_CLASSES = {
  [-2]: "text-lg font-bold tracking-tight md:text-xl",
  [-1]: "text-xl font-bold tracking-tight md:text-2xl",
  [0]: "text-2xl font-bold tracking-tight md:text-3xl",
  [1]: "text-3xl font-bold tracking-tight md:text-4xl xl:text-5xl",
  [2]: "text-4xl font-bold tracking-tight md:text-5xl xl:text-6xl",
} as const;

const AUDIENCE_TEXT_SIZE_CLASSES = {
  [-2]: "text-2xl font-bold tracking-tight md:text-4xl xl:text-5xl",
  [-1]: "text-3xl font-bold tracking-tight md:text-5xl xl:text-6xl",
  [0]: "text-4xl font-bold tracking-tight md:text-6xl xl:text-7xl",
  [1]: "text-5xl font-bold tracking-tight md:text-7xl xl:text-8xl",
  [2]: "text-6xl font-bold tracking-tight md:text-8xl xl:text-8xl",
} as const;

function getLyricsTextSizeClass(
  presentation: "standard" | "audience",
  lyricsFontStep: number,
): string {
  const clampedStep = Math.max(-2, Math.min(2, lyricsFontStep)) as
    | -2
    | -1
    | 0
    | 1
    | 2;

  return presentation === "audience"
    ? AUDIENCE_TEXT_SIZE_CLASSES[clampedStep]
    : STANDARD_TEXT_SIZE_CLASSES[clampedStep];
}

function getActiveWordIndex(words: WordToken[], adjustedMs: number): number {
  let activeIndex = -1;

  for (let index = 0; index < words.length; index += 1) {
    if (words[index].time_ms > adjustedMs) {
      break;
    }
    activeIndex = index;
  }

  return activeIndex;
}

function areLyricLinePropsEqual(
  previous: LyricLineProps,
  next: LyricLineProps,
): boolean {
  if (
    previous.line !== next.line ||
    previous.state !== next.state ||
    previous.presentation !== next.presentation ||
    previous.lyricsFontStep !== next.lyricsFontStep ||
    previous.romanizedText !== next.romanizedText
  ) {
    return false;
  }

  if (previous.state !== "active" && next.state !== "active") {
    return true;
  }

  return previous.adjustedMs === next.adjustedMs;
}

export const LyricLine = memo(function LyricLine({
  line,
  state,
  adjustedMs,
  presentation = "standard",
  lyricsFontStep,
  romanizedText,
}: LyricLineProps) {
  const seek = usePlayerStore((s) => s.seek);
  const isSeekable = state !== "plain";
  const textSizeClass = getLyricsTextSizeClass(presentation, lyricsFontStep);
  const audiencePresentationSpec =
    buildAudiencePresentationSpec(lyricsFontStep);

  const handleClick = () => {
    if (!isSeekable) return;
    seek(line.time_ms);
  };

  const hasWords = line.words !== null && line.words.length > 0;
  const activeWordIndex =
    hasWords && state === "active"
      ? getActiveWordIndex(line.words!, adjustedMs)
      : -1;

  return (
    <div
      onClick={isSeekable ? handleClick : undefined}
      className={`motion-surface flex flex-col items-center gap-1.5 text-center ${
        isSeekable ? "cursor-pointer" : "cursor-default"
      } ${state === "active" ? "scale-105 drop-shadow-md" : ""}`}
      style={
        presentation === "audience"
          ? {
              transform:
                state === "active"
                  ? `scale(${audiencePresentationSpec.activeScale})`
                  : undefined,
            }
          : undefined
      }
    >
      {hasWords ? (
        <span
          className={
            presentation === "audience"
              ? "font-bold tracking-tight"
              : textSizeClass
          }
          style={
            presentation === "audience"
              ? {
                  fontSize: audiencePresentationSpec.fontSizePx,
                  lineHeight: audiencePresentationSpec.lineHeightMultiple,
                }
              : undefined
          }
        >
          {line.words!.map((word, idx) => {
            // When the whole line is past or future, all words use the line-level color
            const wordState =
              state === "plain"
                ? "active"
                : state === "active"
                  ? idx < activeWordIndex
                    ? "past"
                    : idx === activeWordIndex
                      ? "active"
                      : "future"
                  : state === "past"
                    ? "past"
                    : "future";

            return (
              <span
                key={idx}
                className={
                  presentation === "audience"
                    ? "motion-surface"
                    : `motion-surface ${
                        wordState === "active"
                          ? "text-white"
                          : wordState === "past"
                            ? "text-[var(--color-text-dimmer)]"
                            : "text-[var(--color-active)]"
                      }`
                }
                style={
                  presentation === "audience"
                    ? {
                        color: colorToCss(
                          wordState === "active"
                            ? audiencePresentationSpec.activeTextColor
                            : wordState === "past"
                              ? audiencePresentationSpec.pastTextColor
                              : audiencePresentationSpec.futureTextColor,
                        ),
                        textShadow:
                          wordState === "active"
                            ? `0 0 ${audiencePresentationSpec.activeGlowBlurPx}px ${colorToCss(
                                audiencePresentationSpec.activeGlowColor,
                              )}`
                            : undefined,
                      }
                    : wordState === "active"
                      ? {
                          textShadow:
                            "0 0 12px rgba(255,255,255,0.8), 0 0 4px rgba(255,255,255,0.6)",
                        }
                      : undefined
                }
              >
                {word.text}
                {idx < line.words!.length - 1 ? " " : ""}
              </span>
            );
          })}
        </span>
      ) : (
        <span
          className={
            presentation === "audience"
              ? "motion-surface font-bold tracking-tight"
              : `motion-surface ${textSizeClass} ${
                  state === "plain" || state === "active"
                    ? "text-white"
                    : state === "past"
                      ? "text-[var(--color-text-dimmer)]"
                      : "text-[var(--color-active)]"
                }`
          }
          style={
            presentation === "audience"
              ? {
                  fontSize: audiencePresentationSpec.fontSizePx,
                  lineHeight: audiencePresentationSpec.lineHeightMultiple,
                  color: colorToCss(
                    state === "plain" || state === "active"
                      ? audiencePresentationSpec.activeTextColor
                      : state === "past"
                        ? audiencePresentationSpec.pastTextColor
                        : audiencePresentationSpec.futureTextColor,
                  ),
                  textShadow:
                    state === "active"
                      ? `0 0 ${audiencePresentationSpec.activeGlowBlurPx}px ${colorToCss(
                          audiencePresentationSpec.activeGlowColor,
                        )}`
                      : undefined,
                }
              : undefined
          }
        >
          {line.text}
        </span>
      )}
      {romanizedText ? (
        <span
          className={
            presentation === "audience"
              ? "motion-surface font-medium tracking-tight opacity-50"
              : `motion-surface text-sm font-medium md:text-base ${
                  state === "plain" || state === "active"
                    ? "text-[var(--color-text-dim)]"
                    : state === "past"
                      ? "text-[var(--color-text-dimmer)]"
                      : "text-[var(--color-text-dim)]"
                }`
          }
          style={
            presentation === "audience"
              ? {
                  fontSize: audiencePresentationSpec.fontSizePx * 0.55,
                  lineHeight: audiencePresentationSpec.lineHeightMultiple,
                  color: colorToCss(
                    state === "plain" || state === "active"
                      ? audiencePresentationSpec.activeTextColor
                      : state === "past"
                        ? audiencePresentationSpec.pastTextColor
                        : audiencePresentationSpec.futureTextColor,
                  ),
                  opacity: 0.45,
                }
              : undefined
          }
        >
          {romanizedText}
        </span>
      ) : null}
    </div>
  );
}, areLyricLinePropsEqual);
