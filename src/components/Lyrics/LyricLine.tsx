import { usePlayerStore } from "@/stores/player-store";
import type { LyricLine as LyricLineType, WordToken } from "@/types/ipc";

interface LyricLineProps {
  line: LyricLineType;
  state: "active" | "past" | "future";
  adjustedMs: number;
}

function getWordState(
  words: WordToken[],
  index: number,
  adjustedMs: number,
): "past" | "active" | "future" {
  const word = words[index];
  if (word.time_ms > adjustedMs) return "future";
  // This word's time has passed — check if there's a later word that has also started
  const hasLaterWordStarted = words
    .slice(index + 1)
    .some((w) => w.time_ms <= adjustedMs);
  return hasLaterWordStarted ? "past" : "active";
}

export function LyricLine({ line, state, adjustedMs }: LyricLineProps) {
  const seek = usePlayerStore((s) => s.seek);

  const handleClick = () => {
    seek(line.time_ms);
  };

  const hasWords = line.words !== null && line.words.length > 0;

  return (
    <div
      onClick={handleClick}
      className={`motion-surface flex cursor-pointer flex-col items-center gap-1.5 text-center ${
        state === "active" ? "scale-105 drop-shadow-md" : ""
      }`}
    >
      {hasWords ? (
        <span className="text-2xl font-bold tracking-tight md:text-3xl">
          {line.words!.map((word, idx) => {
            // When the whole line is past or future, all words use the line-level color
            const wordState =
              state === "active"
                ? getWordState(line.words!, idx, adjustedMs)
                : state === "past"
                  ? "past"
                  : "future";

            return (
              <span
                key={idx}
                className={`motion-surface ${
                  wordState === "active"
                    ? "text-white"
                    : wordState === "past"
                      ? "text-[var(--color-text-dimmer)]"
                      : "text-[var(--color-active)]"
                }`}
                style={
                  wordState === "active"
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
          className={`motion-surface text-2xl font-bold tracking-tight md:text-3xl ${
            state === "active"
              ? "text-white"
              : state === "past"
                ? "text-[var(--color-text-dimmer)]"
                : "text-[var(--color-active)]"
          }`}
        >
          {line.text}
        </span>
      )}
    </div>
  );
}
