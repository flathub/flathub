import { useEffect, useRef } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { emitTo } from "@tauri-apps/api/event";
import { usePlayerStore } from "@/stores/player-store";
import { useCdgStore } from "@/stores/cdg-store";
import { useLibraryStore } from "@/stores/library-store";
import { drawFrame, clearFrame } from "@/lib/cdg-canvas-painter";
import {
  getCdgSyncChannel,
  postCdgClear,
  postCdgFrame,
  postCdgStatus,
  startCdgSyncRequestListener,
} from "@/lib/cdg-sync-channel";
import { songHasCdgMedia } from "@/lib/song-media";
import * as api from "@/lib/tauri";

// Re-export so CdgCanvas can import from the painter module directly, but
// keep backward compat for any existing callers.
export { setCdgCanvas } from "@/lib/cdg-canvas-painter";

/**
 * PERF: Minimum interval between IPC calls. At 33ms (~30fps) each call
 * transfers ~221KB raw binary via ArrayBuffer. The backend's `changed` flag
 * ensures that frames with no visual updates return an empty body (0 bytes),
 * so increasing the poll rate doesn't proportionally increase IPC load.
 *
 * Previously 66ms (15fps) when using base64, which added O(n) decode overhead
 * per frame. The switch to raw binary removed that bottleneck, allowing 30fps
 * without additional main-thread cost.
 */
const MIN_INTERVAL_MS = 33;

/**
 * Cached state for responding to sync requests from the fullscreen window when
 * it opens mid-song.
 */
let lastFrame: ArrayBuffer | null = null;
let lastStatus: { songId: string | null; hasCdg: boolean } = {
  songId: null,
  hasCdg: false,
};

export function createLatestOnlyFrameForwarder(
  send: (payload: string) => Promise<void>,
): (payload: string) => void {
  let latestPayload: string | null = null;
  let sending = false;

  const flush = () => {
    if (sending || latestPayload === null) {
      return;
    }

    const payload = latestPayload;
    latestPayload = null;
    sending = true;

    void send(payload).finally(() => {
      sending = false;
      flush();
    });
  };

  return (payload: string) => {
    latestPayload = payload;
    flush();
  };
}

const forwardFullscreenFrame = createLatestOnlyFrameForwarder((payload) =>
  emitTo("fullscreen-player", "cdg-frame", payload),
);

/**
 * Convert an ArrayBuffer to a base64 string for the Tauri-event fallback path.
 *
 * PERF: The preferred fullscreen transport is `BroadcastChannel`, which can
 * structured-clone raw `ArrayBuffer` payloads between windows without base64.
 * This conversion stays only for older runtimes where `BroadcastChannel` is
 * unavailable and we must fall back to Tauri's JSON-serialized event API.
 */
/**
 * Normalize the IPC response to an ArrayBuffer.
 *
 * PERF: The backend returns raw bytes via `tauri::ipc::Response`, which
 * **should** arrive as an `ArrayBuffer` on desktop platforms. However, Tauri's
 * IPC bridge may occasionally deliver it as a `number[]` (JSON-serialized
 * Vec<u8>) depending on the protocol path. This function handles both cases
 * so CDG rendering is robust regardless of IPC serialization behavior.
 */
function ensureArrayBuffer(result: unknown): ArrayBuffer {
  if (result instanceof ArrayBuffer) return result;
  if (ArrayBuffer.isView(result)) {
    return result.buffer.slice(
      result.byteOffset,
      result.byteOffset + result.byteLength,
    );
  }
  if (Array.isArray(result)) return new Uint8Array(result).buffer;
  return new ArrayBuffer(0);
}

function arrayBufferToBase64(buffer: ArrayBuffer): string {
  const bytes = new Uint8Array(buffer);
  let binary = "";
  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i]);
  }
  return btoa(binary);
}

function emitCdgFrame(buffer: ArrayBuffer): void {
  lastFrame = buffer;
  const channel = getCdgSyncChannel();
  if (channel) {
    postCdgFrame(channel, buffer);
    return;
  }

  // Convert to base64 for Tauri event JSON serialization
  const base64 = arrayBufferToBase64(buffer);
  forwardFullscreenFrame(base64);
}

function emitCdgClear(): void {
  lastFrame = null;
  const channel = getCdgSyncChannel();
  if (channel) {
    postCdgClear(channel);
    return;
  }

  emitTo("fullscreen-player", "cdg-clear", null).catch(() => {});
}

function emitCdgStatus(songId: string | null, hasCdg: boolean): void {
  lastStatus = { songId, hasCdg };
  const channel = getCdgSyncChannel();
  if (channel) {
    postCdgStatus(channel, { songId, hasCdg });
    return;
  }

  emitTo("fullscreen-player", "cdg-status", { songId, hasCdg }).catch(() => {});
}

export function useCdgSync(enabled = true): void {
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);
  const songs = useLibraryStore((s) => s.songs);
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);
  const pendingRef = useRef(false);
  const currentSongHasCdg = songHasCdgMedia(
    songs.find((song) => song.hash === songId) ?? null,
  );

  // Listen for the fullscreen window requesting the latest cached CDG state.
  useEffect(() => {
    if (!enabled) return;

    const channel = getCdgSyncChannel();
    if (channel) {
      return startCdgSyncRequestListener({
        channel,
        getSnapshot: () => ({
          status: lastStatus,
          frame: lastFrame,
        }),
      });
    }

    let unlisten: UnlistenFn | undefined;
    let cancelled = false;

    listen("cdg-request-sync", () => {
      if (cancelled) return;
      emitTo("fullscreen-player", "cdg-status", lastStatus).catch(() => {});
      if (lastFrame && lastFrame.byteLength > 0) {
        const base64 = arrayBufferToBase64(lastFrame);
        emitTo("fullscreen-player", "cdg-frame", base64).catch(() => {});
      }
    }).then((fn) => {
      if (cancelled) {
        fn();
      } else {
        unlisten = fn;
      }
    });

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [enabled]);

  // Song detection: probe whether the new track has CDG graphics.
  useEffect(() => {
    if (!enabled) return;

    if (!songId) {
      clear();
      clearFrame();
      emitCdgClear();
      emitCdgStatus(null, false);
      return;
    }

    if (!currentSongHasCdg) {
      clear();
      clearFrame();
      emitCdgClear();
      emitCdgStatus(songId, false);
      return;
    }

    let cancelled = false;
    const probePositionMs = usePlayerStore.getState().positionMs;
    const currentCdgSongId = useCdgStore.getState().songId;

    if (currentCdgSongId !== songId) {
      // Clear immediately on song change so a previous song's CDG frame does
      // not linger while we asynchronously probe the new track.
      setSong(songId, true);
      clearFrame();
      emitCdgClear();
      emitCdgStatus(songId, true);
    }

    api
      .getCdgFrame(probePositionMs)
      .then((result) => {
        if (cancelled) return;
        const buffer = ensureArrayBuffer(result);

        if (buffer.byteLength > 0) {
          setSong(songId, true);
          drawFrame(buffer);
          emitCdgFrame(buffer);
          emitCdgStatus(songId, true);
          return;
        }

        emitCdgStatus(songId, true);
      })
      .catch(() => {
        if (cancelled) return;
        setSong(songId, false);
        clearFrame();
        emitCdgStatus(songId, false);
      });

    return () => {
      cancelled = true;
    };
  }, [clear, currentSongHasCdg, enabled, setSong, songId]);

  // Continuous polling: rAF loop that fetches CDG frames during playback.
  useEffect(() => {
    if (!enabled) return;

    const stopPolling = startCdgPollingLoop(() => {
      const { snapshot, positionMs } = usePlayerStore.getState();
      const { hasCdg } = useCdgStore.getState();

      if (!hasCdg || !snapshot?.is_playing || pendingRef.current) {
        return;
      }
      pendingRef.current = true;

      // PERF: The hot frame path stays out of React state. The IPC returns a
      // raw ArrayBuffer (no base64), and drawFrame() paints it to a pre-
      // allocated ImageData — no string decoding, no per-frame allocation.
      api
        .getCdgFrame(positionMs)
        .then((result) => {
          const buffer = ensureArrayBuffer(result);
          if (buffer.byteLength > 0) {
            drawFrame(buffer);
            emitCdgFrame(buffer);
          }
        })
        .catch(() => {
          // Silently ignore CDG frame errors — non-critical for playback.
        })
        .finally(() => {
          pendingRef.current = false;
        });
    });

    return () => {
      stopPolling();
    };
  }, [enabled]);
}

export function startCdgPollingLoop(
  tick: () => void,
  timers: Pick<typeof globalThis, "setInterval" | "clearInterval"> = globalThis,
): () => void {
  const timer = timers.setInterval(tick, MIN_INTERVAL_MS);
  return () => timers.clearInterval(timer);
}
