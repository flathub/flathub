import { useEffect } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { emitTo } from "@tauri-apps/api/event";
import { drawFrameFromBase64, clearFrame } from "@/lib/cdg-canvas-painter";
import { useCdgStore } from "@/stores/cdg-store";

/**
 * Fullscreen-window counterpart to `useCdgSync`. Instead of polling
 * `getCdgFrame()` from the backend (which would conflict with the main
 * window's mutex-based state tracking), this hook receives CDG frames
 * forwarded by the main window via Tauri events.
 *
 * On mount it emits `cdg-request-sync` so the main window re-sends its
 * cached frame and status — this handles the case where the fullscreen
 * window opens mid-song.
 *
 * PERF: Frames arrive as base64 strings because Tauri's event API uses JSON
 * serialization, which cannot carry raw `ArrayBuffer` payloads. This is
 * acceptable for the fullscreen window (secondary display). The main window
 * uses the raw binary `drawFrame(ArrayBuffer)` path for better performance.
 * See `cdg-canvas-painter.ts` for details on the two rendering paths.
 */
export function useCdgFrameReceiver(): void {
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);

  useEffect(() => {
    let cancelled = false;
    const unlisteners: UnlistenFn[] = [];

    // Request current CDG state from the main window.
    emitTo("main", "cdg-request-sync", null).catch(() => {});

    listen<string>("cdg-frame", (event) => {
      if (!cancelled) drawFrameFromBase64(event.payload);
    }).then((fn) => {
      if (cancelled) fn();
      else unlisteners.push(fn);
    });

    listen("cdg-clear", () => {
      if (!cancelled) {
        clear();
        clearFrame();
      }
    }).then((fn) => {
      if (cancelled) fn();
      else unlisteners.push(fn);
    });

    listen<{ songId: string | null; hasCdg: boolean }>(
      "cdg-status",
      (event) => {
        if (!cancelled) {
          const { songId, hasCdg } = event.payload;
          if (songId !== null) {
            setSong(songId, hasCdg);
          } else {
            clear();
          }
        }
      },
    ).then((fn) => {
      if (cancelled) fn();
      else unlisteners.push(fn);
    });

    return () => {
      cancelled = true;
      for (const fn of unlisteners) fn();
    };
  }, [clear, setSong]);
}
