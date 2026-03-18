import { useEffect } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { emitTo } from "@tauri-apps/api/event";
import {
  clearFrame,
  drawFrame,
  drawFrameFromBase64,
} from "@/lib/cdg-canvas-painter";
import {
  getCdgSyncChannel,
  startCdgSyncReceiver,
} from "@/lib/cdg-sync-channel";
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
 * forwarded by the main window.
 *
 * On mount it requests a sync so the main window re-sends its cached frame and
 * status — this handles the case where the fullscreen window opens mid-song.
 *
 * PERF: The preferred path uses `BroadcastChannel`, so the fullscreen window
 * receives raw `ArrayBuffer` frames and can paint them with the same binary
 * path as the main window. A base64 Tauri-event fallback remains for runtimes
 * without `BroadcastChannel` support.
 */
/**
 * Create rAF-gated frame painting callbacks.
 *
 * Instead of painting every incoming frame immediately, we buffer the latest
 * frame and paint it on the next `requestAnimationFrame`. This:
 * - Naturally drops intermediate frames (only the latest is painted per vsync)
 * - Syncs painting to the display refresh rate
 * - Eliminates queue buildup from BroadcastChannel message delivery jitter
 */
function createRafGatedPainter<T>(paint: (frame: T) => void): {
  enqueue: (frame: T) => void;
  cancel: () => void;
} {
  let latestFrame: T | null = null;
  let rafId = 0;

  const flush = () => {
    rafId = 0;
    if (latestFrame !== null) {
      paint(latestFrame);
      latestFrame = null;
    }
  };

  return {
    enqueue: (frame: T) => {
      latestFrame = frame;
      if (!rafId) {
        rafId = requestAnimationFrame(flush);
      }
    },
    cancel: () => {
      if (rafId) {
        cancelAnimationFrame(rafId);
        rafId = 0;
      }
      latestFrame = null;
    },
  };
}

export function useCdgFrameReceiver(): void {
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);

  useEffect(() => {
    const channel = getCdgSyncChannel();
    if (channel) {
      const painter = createRafGatedPainter<ArrayBuffer>(drawFrame);
      const stopReceiver = startCdgSyncReceiver({
        channel,
        onFrame: (payload) => {
          painter.enqueue(payload);
        },
        onClear: () => {
          painter.cancel();
          clear();
          clearFrame();
        },
        onStatus: ({ songId, hasCdg }) => {
          if (songId !== null) {
            setSong(songId, hasCdg);
          } else {
            clear();
          }
        },
      });

      return () => {
        painter.cancel();
        stopReceiver();
      };
    }

    let cancelled = false;
    let unlisteners: UnlistenFn[] = [];
    const painter = createRafGatedPainter<string>(drawFrameFromBase64);

    void startCdgFrameReceiver({
      listen,
      emitSyncRequest: () => emitTo("main", "cdg-request-sync", null),
      onFrame: (payload) => {
        if (!cancelled) {
          painter.enqueue(payload);
        }
      },
      onClear: () => {
        if (!cancelled) {
          painter.cancel();
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
      painter.cancel();
      for (const fn of unlisteners) fn();
    };
  }, [clear, setSong]);
}
