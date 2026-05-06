import { useEffect, useRef, type RefObject } from "react";
import { getCenteredScrollTop } from "@/components/Lyrics/lyrics-scroll";

// Duration (ms) to suppress auto-scroll after the user manually scrolls.
// Long enough to let users read ahead without being yanked back immediately.
const USER_SCROLL_PAUSE_MS = 3000;

/**
 * Attaches wheel and touchstart listeners to a container element and tracks
 * whether the user has recently scrolled manually. Returns an object with an
 * `isActive()` predicate (true while the pause window is open) and a
 * `destroy()` cleanup that removes listeners and clears any pending timer.
 *
 * Exported so the suppression logic can be exercised in isolation without
 * needing a React renderer.
 *
 * Wheel and touchstart fire only on genuine user interaction — programmatic
 * scrollTo() does not trigger them — so no extra flag is needed to tell
 * auto-scrolls apart from manual ones.
 */
export function createUserScrollGuard(
  container: HTMLElement,
  pauseMs: number,
  timers: {
    setTimeout: typeof globalThis.setTimeout;
    clearTimeout: typeof globalThis.clearTimeout;
  } = { setTimeout, clearTimeout },
): { isActive: () => boolean; destroy: () => void } {
  let scrolling = false;
  let timer: ReturnType<typeof setTimeout> | null = null;

  const onUserScroll = () => {
    scrolling = true;
    if (timer !== null) timers.clearTimeout(timer);
    timer = timers.setTimeout(() => {
      scrolling = false;
    }, pauseMs);
  };

  container.addEventListener("wheel", onUserScroll, { passive: true });
  container.addEventListener("touchstart", onUserScroll, { passive: true });

  return {
    isActive: () => scrolling,
    destroy: () => {
      container.removeEventListener("wheel", onUserScroll);
      container.removeEventListener("touchstart", onUserScroll);
      if (timer !== null) timers.clearTimeout(timer);
      scrolling = false;
    },
  };
}

/**
 * Scrolls the lyrics container so the currently active line is vertically
 * centered. Disabled for plain-text (un-synced) lyrics where there is no
 * meaningful active line to track.
 *
 * When the user manually scrolls (wheel or touch), auto-scroll is suppressed
 * for USER_SCROLL_PAUSE_MS so they can preview upcoming lyrics without
 * being immediately pulled back to the current line.
 */
export function useLyricsAutoScroll(
  containerRef: RefObject<HTMLDivElement | null>,
  activeLineIndex: number,
  isPlainText: boolean,
  lyricsFontStep: number,
  presentation: "standard" | "audience",
  songId: string | null | undefined,
  layoutVersion = "",
): void {
  const guardRef = useRef<ReturnType<typeof createUserScrollGuard> | null>(
    null,
  );

  // Attach user-scroll detection. Re-runs when the song changes so the guard
  // is always scoped to the current container and starts fresh (no stale pause
  // window carrying over from a previous track).
  useEffect(() => {
    if (isPlainText) return;
    const container = containerRef.current;
    if (!container) return;

    const guard = createUserScrollGuard(container, USER_SCROLL_PAUSE_MS);
    guardRef.current = guard;

    return () => {
      guard.destroy();
      guardRef.current = null;
    };
  }, [containerRef, isPlainText, songId]);

  // Scroll to center the active line, skipping if the user has recently
  // scrolled manually.
  useEffect(() => {
    if (isPlainText) return;
    if (activeLineIndex < 0 || !containerRef.current) return;
    if (guardRef.current?.isActive()) return;

    const lineEl = containerRef.current.querySelector<HTMLElement>(
      `[data-lyrics-line-index="${activeLineIndex}"]`,
    );
    if (!lineEl) return;

    const top = getCenteredScrollTop({
      viewportHeight: containerRef.current.clientHeight,
      scrollHeight: containerRef.current.scrollHeight,
      lineOffsetTop: lineEl.offsetTop,
      lineHeight: lineEl.clientHeight,
    });

    containerRef.current.scrollTo({
      top,
      behavior: "smooth",
    });
  }, [
    containerRef,
    activeLineIndex,
    isPlainText,
    lyricsFontStep,
    presentation,
    songId,
    layoutVersion,
  ]);
}
