import { useEffect, useRef } from "react";
import { usePlayerStore } from "@/stores/player-store";
import { useCdgStore } from "@/stores/cdg-store";
import { useLibraryStore } from "@/stores/library-store";
import { drawFrame, clearFrame } from "@/lib/cdg-canvas-painter";
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
 */
const MIN_INTERVAL_MS = 33;

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

export function useCdgSync(enabled = true): void {
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);
  const songs = useLibraryStore((s) => s.songs);
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);
  const pendingRef = useRef(false);
  const currentSongHasCdg = songHasCdgMedia(
    songs.find((song) => song.hash === songId) ?? null,
  );

  // Song detection: probe whether the new track has CDG graphics.
  useEffect(() => {
    if (!enabled) return;

    if (!songId) {
      clear();
      clearFrame();
      return;
    }

    if (!currentSongHasCdg) {
      clear();
      clearFrame();
      return;
    }

    let cancelled = false;
    const probePositionMs = usePlayerStore.getState().positionMs;
    const currentCdgSongId = useCdgStore.getState().songId;

    if (currentCdgSongId !== songId) {
      setSong(songId, true);
      clearFrame();
    }

    api
      .getCdgFrame(probePositionMs)
      .then((result) => {
        if (cancelled) return;
        const buffer = ensureArrayBuffer(result);

        if (buffer.byteLength > 0) {
          setSong(songId, true);
          drawFrame(buffer);
          return;
        }
      })
      .catch(() => {
        if (cancelled) return;
        setSong(songId, false);
        clearFrame();
      });

    return () => {
      cancelled = true;
    };
  }, [clear, currentSongHasCdg, enabled, setSong, songId]);

  // Continuous polling: fetches CDG frames during playback.
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
