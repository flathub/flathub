import { useEffect } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { emitTo } from "@tauri-apps/api/event";
import { drawFrameFromBase64, clearFrame } from "@/lib/cdg-canvas-painter";
import { useCdgStore } from "@/stores/cdg-store";

interface StartCdgFrameReceiverOptions {
  listen: typeof listen;
  emitSyncRequest: () => Promise<void>;
  onFrame: (payload: string) => void;
  onClear: () => void;
  onStatus: (payload: { songId: string | null; hasCdg: boolean }) => void;
}

export async function startCdgFrameReceiver({
  listen,
  emitSyncRequest,
  onFrame,
  onClear,
  onStatus,
}: StartCdgFrameReceiverOptions): Promise<UnlistenFn[]> {
  const unlisteners = await Promise.all([
    listen<string>("cdg-frame", (event) => {
      onFrame(event.payload);
    }),
    listen("cdg-clear", () => {
      onClear();
    }),
    listen<{ songId: string | null; hasCdg: boolean }>(
      "cdg-status",
      (event) => {
        onStatus(event.payload);
      },
    ),
  ]);

  await emitSyncRequest();
  return unlisteners;
}

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
    let unlisteners: UnlistenFn[] = [];

    void startCdgFrameReceiver({
      listen,
      emitSyncRequest: () => emitTo("main", "cdg-request-sync", null),
      onFrame: (payload) => {
        if (!cancelled) {
          drawFrameFromBase64(payload);
        }
      },
      onClear: () => {
        if (!cancelled) {
          clear();
          clearFrame();
        }
      },
      onStatus: ({ songId, hasCdg }) => {
        if (!cancelled) {
          if (songId !== null) {
            setSong(songId, hasCdg);
          } else {
            clear();
          }
        }
      },
    })
      .then((listeners) => {
        if (cancelled) {
          for (const fn of listeners) fn();
        } else {
          unlisteners = listeners;
        }
      })
      .catch(() => {});

    return () => {
      cancelled = true;
      for (const fn of unlisteners) fn();
    };
  }, [clear, setSong]);
}
