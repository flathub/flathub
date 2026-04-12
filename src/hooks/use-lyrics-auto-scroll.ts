import { useEffect, type RefObject } from "react";
import { getCenteredScrollTop } from "@/components/Lyrics/lyrics-scroll";

/**
 * Scrolls the lyrics container so the currently active line is vertically
 * centered. Disabled for plain-text (un-synced) lyrics where there is no
 * meaningful active line to track.
 */
export function useLyricsAutoScroll(
  containerRef: RefObject<HTMLDivElement | null>,
  activeLineIndex: number,
  isPlainText: boolean,
  lyricsFontStep: number,
  presentation: "standard" | "audience",
  songId: string | null | undefined,
): void {
  useEffect(() => {
    if (isPlainText) return;
    if (activeLineIndex < 0 || !containerRef.current) return;

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
  ]);
}
