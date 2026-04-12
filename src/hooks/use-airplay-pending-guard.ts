import { useEffect, useRef } from "react";
import { usePlayerStore } from "@/stores/player-store";

/**
 * Clears the AirPlay plain-text page-pending flag when conditions invalidate
 * it — for example when the song changes, lyrics switch away from plain text,
 * or the AirPlay remote target is no longer active.
 */
export function useAirPlayPendingGuard(
  songId: string | null | undefined,
  isPlainText: boolean,
  isAudience: boolean,
  isAirPlayRemotePagingTarget: boolean,
  airPlayPlainTextPagePending: boolean,
): void {
  const lastPendingSongIdRef = useRef<string | null>(null);

  useEffect(() => {
    if (!airPlayPlainTextPagePending) {
      lastPendingSongIdRef.current = songId ?? null;
      return;
    }

    const songChanged =
      lastPendingSongIdRef.current !== null &&
      lastPendingSongIdRef.current !== (songId ?? null);
    if (
      isAudience ||
      songChanged ||
      !isPlainText ||
      !isAirPlayRemotePagingTarget
    ) {
      usePlayerStore.getState().clearAirPlayPlainTextPagePending();
      return;
    }

    lastPendingSongIdRef.current = songId ?? null;
  }, [
    airPlayPlainTextPagePending,
    isAirPlayRemotePagingTarget,
    isAudience,
    isPlainText,
    songId,
  ]);
}
