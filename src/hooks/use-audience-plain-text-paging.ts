import { listen } from "@tauri-apps/api/event";
import {
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
  type RefObject,
} from "react";
import {
  LOCAL_AUDIENCE_PLAIN_TEXT_PAGE_EVENT,
  type PlainTextPageDirection,
} from "@/lib/plain-text-page-controls";
import { buildPlainTextPageStartIndices } from "@/components/Lyrics/plain-text-pages";
import type { AudiencePresentationSpec } from "@/types/ipc";
import type { LyricLine } from "@/types/ipc";

function arePageStartIndicesEqual(left: number[], right: number[]): boolean {
  return (
    left.length === right.length &&
    left.every((value, index) => value === right[index])
  );
}

interface AudiencePlainTextPagingInput {
  lines: LyricLine[];
  shouldRender: boolean;
  pageIdentity: string;
  audiencePresentationSpec: AudiencePresentationSpec;
}

interface AudiencePlainTextPagingResult {
  containerRef: RefObject<HTMLDivElement | null>;
  measurementRef: RefObject<HTMLDivElement | null>;
  pageState: { identity: string; index: number };
  pageStartIndices: number[];
  pageIndex: number;
  currentPageStart: number;
  currentPageEnd: number;
  visibleLines: LyricLine[];
}

/**
 * Manages audience-mode plain-text paging: measures line heights to build
 * page boundaries, listens for page-step events, and exposes the current
 * page slice of lyrics lines.
 */
export function useAudiencePlainTextPaging({
  lines,
  shouldRender,
  pageIdentity,
  audiencePresentationSpec,
}: AudiencePlainTextPagingInput): AudiencePlainTextPagingResult {
  const containerRef = useRef<HTMLDivElement>(null);
  const measurementRef = useRef<HTMLDivElement>(null);
  const [pageStartIndices, setPageStartIndices] = useState<number[]>([0]);
  const [pageState, setPageState] = useState({
    identity: pageIdentity,
    index: 0,
  });

  const pageIndex = pageState.identity === pageIdentity ? pageState.index : 0;
  const currentPageStart = shouldRender
    ? (pageStartIndices[pageIndex] ?? 0)
    : 0;
  const currentPageEnd = shouldRender
    ? (pageStartIndices[pageIndex + 1] ?? lines.length)
    : lines.length;
  const visibleLines = shouldRender
    ? lines.slice(currentPageStart, currentPageEnd)
    : lines;

  useLayoutEffect(() => {
    if (!containerRef.current || !measurementRef.current) {
      return;
    }

    const measurePages = () => {
      if (!containerRef.current || !measurementRef.current) {
        return;
      }

      const lineHeights = Array.from(
        measurementRef.current.querySelectorAll<HTMLElement>(
          "[data-plain-text-page-measure-line]",
        ),
      ).map((line) =>
        Math.max(1, Math.ceil(line.getBoundingClientRect().height)),
      );
      const availableHeight = Math.max(
        1,
        containerRef.current.clientHeight -
          audiencePresentationSpec.verticalPaddingPx * 2,
      );
      const nextPageStartIndices = buildPlainTextPageStartIndices(
        lineHeights,
        availableHeight,
        audiencePresentationSpec.lineGapPx,
      );

      setPageStartIndices((current) =>
        arePageStartIndicesEqual(current, nextPageStartIndices)
          ? current
          : nextPageStartIndices,
      );
      setPageState((current) => {
        const activeIndex =
          current.identity === pageIdentity ? current.index : 0;

        return {
          identity: pageIdentity,
          index: Math.max(
            0,
            Math.min(activeIndex, nextPageStartIndices.length - 1),
          ),
        };
      });
    };

    if (!shouldRender) {
      return;
    }

    measurePages();

    if (typeof ResizeObserver === "undefined") {
      return;
    }

    const observer = new ResizeObserver(() => {
      measurePages();
    });
    observer.observe(containerRef.current);

    return () => {
      observer.disconnect();
    };
  }, [
    audiencePresentationSpec.lineGapPx,
    audiencePresentationSpec.verticalPaddingPx,
    lines,
    pageIdentity,
    shouldRender,
  ]);

  useEffect(() => {
    if (!shouldRender) {
      return;
    }

    let cancelled = false;
    let unlisten: (() => void) | null = null;

    const setup = async () => {
      unlisten = await listen<{ direction: PlainTextPageDirection }>(
        LOCAL_AUDIENCE_PLAIN_TEXT_PAGE_EVENT,
        (event) => {
          if (cancelled) {
            return;
          }

          const delta = event.payload.direction === "prev" ? -1 : 1;
          setPageState((current) => {
            const activeIndex =
              current.identity === pageIdentity ? current.index : 0;

            return {
              identity: pageIdentity,
              index: Math.max(
                0,
                Math.min(activeIndex + delta, pageStartIndices.length - 1),
              ),
            };
          });
        },
      );
    };

    void setup();

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [pageIdentity, pageStartIndices.length, shouldRender]);

  return {
    containerRef,
    measurementRef,
    pageState,
    pageStartIndices,
    pageIndex,
    currentPageStart,
    currentPageEnd,
    visibleLines,
  };
}
