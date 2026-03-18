import { useEffect, useRef } from "react";
import { usePlayerStore } from "@/stores/player-store";
import { useCdgStore } from "@/stores/cdg-store";
import { drawFrame, clearFrame } from "@/lib/cdg-canvas-painter";
import { getCdgDisplayFrame } from "@/lib/tauri";

/** Must match the polling interval in use-cdg-sync.ts. */
const POLL_INTERVAL_MS = 33;

/** Size of the version header prepended to display-frame responses (u64 LE). */
const VERSION_HEADER_BYTES = 8;

/**
 * Normalize the IPC response to an ArrayBuffer.
 *
 * PERF: The backend returns raw bytes via `tauri::ipc::Response`, which
 * **should** arrive as an `ArrayBuffer` on desktop platforms. However, Tauri's
 * IPC bridge may occasionally deliver it as a `number[]` (JSON-serialized
 * Vec<u8>) depending on the protocol path. This function handles both cases.
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

/**
 * Parse the version header (u64 little-endian) from the first 8 bytes of the
 * display-frame response. Returns `0` for empty/invalid buffers.
 */
function readVersion(buffer: ArrayBuffer): number {
  if (buffer.byteLength < VERSION_HEADER_BYTES) return 0;
  const view = new DataView(buffer);
  // u64 LE — we only use the lower 32 bits (safe for billions of frames).
  return view.getUint32(0, true);
}

/**
 * Fullscreen-window counterpart to `useCdgSync`.
 *
 * Instead of advancing the CDG renderer (which the main window owns), this
 * hook polls `get_cdg_display_frame` — a read-only command that returns the
 * latest cached frame from the renderer without mutating state.
 *
 * The response carries a version header so that unchanged frames are skipped
 * (only the 8-byte header is returned). This drops per-poll IPC cost from
 * 221 KB to 8 bytes when the visual frame hasn't changed.
 */
export function useCdgFrameReceiver(): void {
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);
  const pendingRef = useRef(false);
  const lastVersionRef = useRef(0);

  useEffect(() => {
    const timer = setInterval(() => {
      const { snapshot } = usePlayerStore.getState();
      const songId = snapshot?.song_id ?? null;

      if (!songId || !snapshot?.is_playing || pendingRef.current) {
        return;
      }

      pendingRef.current = true;

      getCdgDisplayFrame(lastVersionRef.current)
        .then((result) => {
          const buffer = ensureArrayBuffer(result);

          // Empty buffer → no CDG active.
          if (buffer.byteLength === 0) return;

          const version = readVersion(buffer);
          lastVersionRef.current = version;

          // Version-only header (8 bytes) → frame unchanged, skip paint.
          if (buffer.byteLength <= VERSION_HEADER_BYTES) return;

          // New frame: strip version header, paint RGBA data.
          const frameData = buffer.slice(VERSION_HEADER_BYTES);
          drawFrame(frameData);

          // Ensure CDG store reflects that we have active CDG content.
          const { hasCdg } = useCdgStore.getState();
          if (!hasCdg) {
            setSong(songId, true);
          }
        })
        .catch(() => {
          // Silently ignore — non-critical for playback.
        })
        .finally(() => {
          pendingRef.current = false;
        });
    }, POLL_INTERVAL_MS);

    return () => {
      clearInterval(timer);
    };
  }, [setSong]);

  // Clear CDG state when the song changes or stops.
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);

  useEffect(() => {
    if (!songId) {
      clear();
      clearFrame();
      lastVersionRef.current = 0;
    }
  }, [clear, songId]);
}
