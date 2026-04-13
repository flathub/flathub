import { useEffect, useRef } from "react";
import {
  selectSyncDisplayPositionMs,
  usePlayerStore,
} from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";

const LYRICS_SYNC_INTERVAL_MS = 33;

export function syncLyricsToPlayback(prevIndexRef: { current: number }) {
  const state = usePlayerStore.getState();
  const { snapshot } = state;
  const { lines, offsetMs, setActiveLineIndex } = useLyricsStore.getState();

  // Allow sync when paused so seek-while-paused immediately updates the active line.
  // Guard only against no song loaded or no lines to sync.
  if (!snapshot?.song_id || lines.length === 0) {
    return;
  }

  const positionMs = selectSyncDisplayPositionMs(state);
  const adjustedMs = positionMs - offsetMs;
  const index = binarySearchLine(lines, adjustedMs);

  if (index !== prevIndexRef.current) {
    prevIndexRef.current = index;
    setActiveLineIndex(index);
  }
}

export function startLyricsSyncLoop(
  tick: () => void,
  timers: Pick<typeof globalThis, "setInterval" | "clearInterval"> = globalThis,
): () => void {
  const timer = timers.setInterval(tick, LYRICS_SYNC_INTERVAL_MS);
  return () => timers.clearInterval(timer);
}

export function useLyricsSync(enabled = true): void {
  const prevIndexRef = useRef(-1);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    const stopLoop = startLyricsSyncLoop(() =>
      syncLyricsToPlayback(prevIndexRef),
    );

    // Force-sync lyrics when window regains focus so the current line snaps
    // into place immediately after backgrounding or monitor changes.
    const syncNow = () => {
      syncLyricsToPlayback(prevIndexRef);
    };
    window.addEventListener("focus", syncNow);

    return () => {
      stopLoop();
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
