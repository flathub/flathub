import { useEffect, useRef } from "react";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";

export function useLyricsSync(enabled = true): void {
  const rafRef = useRef<number>(0);
  const prevIndexRef = useRef(-1);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    const tick = () => {
      const { snapshot, positionMs } = usePlayerStore.getState();
      const { lines, offsetMs, setActiveLineIndex } = useLyricsStore.getState();

      if (!snapshot?.is_playing || lines.length === 0) {
        rafRef.current = requestAnimationFrame(tick);
        return;
      }

      const adjustedMs = positionMs - offsetMs;
      const index = binarySearchLine(lines, adjustedMs);

      if (index !== prevIndexRef.current) {
        prevIndexRef.current = index;
        setActiveLineIndex(index);
      }

      rafRef.current = requestAnimationFrame(tick);
    };

    rafRef.current = requestAnimationFrame(tick);

    // Force-sync lyrics when window regains focus (macOS throttles rAF
    // for background windows, so activeLineIndex can fall behind).
    const syncNow = () => {
      const { positionMs } = usePlayerStore.getState();
      const { lines, offsetMs, setActiveLineIndex } = useLyricsStore.getState();
      if (lines.length === 0) return;
      const index = binarySearchLine(lines, positionMs - offsetMs);
      prevIndexRef.current = index;
      setActiveLineIndex(index);
    };
    window.addEventListener("focus", syncNow);

    return () => {
      cancelAnimationFrame(rafRef.current);
      window.removeEventListener("focus", syncNow);
    };
  }, [enabled]);
}

function binarySearchLine(
  lines: { time_ms: number }[],
  currentMs: number,
): number {
  let lo = 0;
  let hi = lines.length - 1;
  let result = -1;

  while (lo <= hi) {
    const mid = (lo + hi) >>> 1;
    if (lines[mid].time_ms <= currentMs) {
      result = mid;
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }

  return result;
}
