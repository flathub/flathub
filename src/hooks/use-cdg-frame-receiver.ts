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
 * Create a coalescing frame painter that buffers the latest frame and paints it
 * on the next macrotask via `setTimeout(0)`.
 *
 * IMPORTANT: We deliberately avoid `requestAnimationFrame` here because macOS
 * throttles (or completely suspends) rAF callbacks for non-focused windows.
 * The fullscreen player window is typically unfocused (the user interacts with
 * the main window), so rAF would cause the CDG canvas to freeze.
 *
 * `setTimeout(0)` coalesces multiple BroadcastChannel messages that arrive in
 * the same event-loop tick into a single paint, dropping intermediate frames.
 * It fires reliably regardless of window focus state.
 */
function createCoalescingPainter<T>(paint: (frame: T) => void): {
  enqueue: (frame: T) => void;
  cancel: () => void;
} {
  let latestFrame: T | null = null;
  let timerId: ReturnType<typeof setTimeout> | null = null;

  const flush = () => {
    timerId = null;
    if (latestFrame !== null) {
      paint(latestFrame);
      latestFrame = null;
    }
  };

  return {
    enqueue: (frame: T) => {
      latestFrame = frame;
      if (timerId === null) {
        timerId = setTimeout(flush, 0);
      }
    },
    cancel: () => {
      if (timerId !== null) {
        clearTimeout(timerId);
        timerId = null;
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
      const painter = createCoalescingPainter<ArrayBuffer>(drawFrame);
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
    const painter = createCoalescingPainter<string>(drawFrameFromBase64);

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
