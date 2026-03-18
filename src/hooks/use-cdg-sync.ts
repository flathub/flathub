import { useEffect, useRef } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { emitTo } from "@tauri-apps/api/event";
import { usePlayerStore } from "@/stores/player-store";
import { useCdgStore } from "@/stores/cdg-store";
import { drawFrame, clearFrame } from "@/lib/cdg-canvas-painter";
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
 * Cached state for responding to `cdg-request-sync` from the fullscreen window
 * when it opens mid-song.
 */
let lastFrame: ArrayBuffer | null = null;
let lastStatus: { songId: string | null; hasCdg: boolean } = {
  songId: null,
  hasCdg: false,
};

/**
 * Convert an ArrayBuffer to a base64 string for Tauri event forwarding.
 *
 * PERF: This conversion is intentionally limited to the event forwarding path
 * (main window → fullscreen window). Tauri's event API uses JSON serialization,
 * which cannot carry raw `ArrayBuffer` payloads, so base64 is unavoidable here.
 * The main window's own rendering path uses raw `ArrayBuffer` directly via
 * `drawFrame()` and never touches base64. Do not "optimize" by caching base64
 * on the main rendering path — it would reintroduce the bottleneck we removed.
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
  // Convert to base64 for Tauri event JSON serialization
  const base64 = arrayBufferToBase64(buffer);
  emitTo("fullscreen-player", "cdg-frame", base64).catch(() => {});
}

function emitCdgClear(): void {
  lastFrame = null;
  emitTo("fullscreen-player", "cdg-clear", null).catch(() => {});
}

function emitCdgStatus(songId: string | null, hasCdg: boolean): void {
  lastStatus = { songId, hasCdg };
  emitTo("fullscreen-player", "cdg-status", { songId, hasCdg }).catch(() => {});
}

export function useCdgSync(enabled = true): void {
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);
  const currentCdgSongId = useCdgStore((s) => s.songId);
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);
  const rafRef = useRef<number>(0);
  const lastCallRef = useRef(0);
  const pendingRef = useRef(false);

  // Listen for fullscreen window requesting current CDG state on mount.
  useEffect(() => {
    if (!enabled) return;

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

    let cancelled = false;
    const probePositionMs = usePlayerStore.getState().positionMs;

    if (currentCdgSongId !== songId) {
      // Clear immediately on song change so a previous song's CDG frame does
      // not linger while we asynchronously probe the new track.
      setSong(songId, false);
      clearFrame();
      emitCdgClear();
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
          lastCallRef.current = 0;
          return;
        }

        setSong(songId, false);
        clearFrame();
        emitCdgStatus(songId, false);
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
  }, [clear, currentCdgSongId, enabled, setSong, songId]);

  // Continuous polling: rAF loop that fetches CDG frames during playback.
  useEffect(() => {
    if (!enabled) return;

    const tick = () => {
      const { snapshot, positionMs } = usePlayerStore.getState();
      const { hasCdg } = useCdgStore.getState();

      if (!hasCdg || !snapshot?.is_playing) {
        rafRef.current = requestAnimationFrame(tick);
        return;
      }

      const now = performance.now();
      if (now - lastCallRef.current < MIN_INTERVAL_MS || pendingRef.current) {
        rafRef.current = requestAnimationFrame(tick);
        return;
      }

      lastCallRef.current = now;
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

      rafRef.current = requestAnimationFrame(tick);
    };

    rafRef.current = requestAnimationFrame(tick);

    return () => {
      cancelAnimationFrame(rafRef.current);
    };
  }, [enabled]);
}
