import { useEffect, useRef } from "react";
import {
  selectAudiencePreviewPositionMs,
  usePlayerStore,
} from "@/stores/player-store";
import { useCdgStore } from "@/stores/cdg-store";
import { useLibraryStore } from "@/stores/library-store";
import { drawFrame, clearFrame } from "@/lib/cdg-canvas-painter";
import {
  getCdgSyncChannel,
  postCdgClear,
  postCdgFrame,
  postCdgStatus,
  startCdgSyncRequestListener,
  type CdgSyncStatusPayload,
} from "@/lib/cdg-sync-channel";
import { songHasCdgMedia } from "@/lib/song-media";
import * as api from "@/lib/tauri";

// Re-export so CdgCanvas can import from the painter module directly, but
// keep backward compat for any existing callers.
export { setCdgCanvas } from "@/lib/cdg-canvas-painter";

/**
 * Target cadence for CDG frame fetches. We no longer rely on JS timers here,
 * because macOS can throttle them in occluded windows; instead we map backend
 * playback-position events into 33ms buckets and fetch once per bucket.
 *
 * RATIONALE: This is part of the second-window CDG fix, not redundant code.
 * When the audience window covers most of the main window, macOS can throttle
 * the main window's JS timers toward slideshow cadence. The main window must
 * therefore advance CDG from Rust playback-position events, then publish those
 * frames to the second window over BroadcastChannel.
 */
const MIN_INTERVAL_MS = 33;

let lastFrame: ArrayBuffer | null = null;
let lastStatus: CdgSyncStatusPayload = {
  songId: null,
  hasCdg: false,
};

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

function emitCdgFrame(buffer: ArrayBuffer): void {
  lastFrame = buffer;
  postCdgFrame(getCdgSyncChannel(), buffer);
}

function emitCdgClear(): void {
  lastFrame = null;
  postCdgClear(getCdgSyncChannel());
}

function emitCdgStatus(songId: string | null, hasCdg: boolean): void {
  lastStatus = { songId, hasCdg };
  postCdgStatus(getCdgSyncChannel(), lastStatus);
}

function getCdgSyncBucket(positionMs: number): number {
  return Math.floor(Math.max(0, positionMs) / MIN_INTERVAL_MS);
}

export function startCdgPositionSync(
  tick: () => void,
  subscribe: (
    listener: (positionMs: number, previousPositionMs: number) => void,
  ) => () => void,
): () => void {
  return subscribe((positionMs, previousPositionMs) => {
    if (getCdgSyncBucket(positionMs) !== getCdgSyncBucket(previousPositionMs)) {
      tick();
    }
  });
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

  useEffect(() => {
    if (!enabled) return;

    const channel = getCdgSyncChannel();
    if (!channel) {
      return;
    }

    return startCdgSyncRequestListener({
      channel,
      getSnapshot: () => ({
        status: lastStatus,
        frame: lastFrame,
      }),
    });
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
    const probePositionMs = selectAudiencePreviewPositionMs(
      usePlayerStore.getState(),
    );
    const currentCdgSongId = useCdgStore.getState().songId;

    if (currentCdgSongId !== songId) {
      // Clear immediately on song change so the audience window cannot keep
      // showing the previous song while the new track's first frame arrives.
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

  // RATIONALE: Do not replace this with setInterval/requestAnimationFrame.
  // The real regression was macOS throttling front-end scheduling in windows
  // that are heavily occluded by the audience display. Keeping the fetch loop
  // tied to Rust playback-position events is what preserves smooth CDG in both
  // windows.
  useEffect(() => {
    if (!enabled) return;

    const stopSync = startCdgPositionSync(
      () => {
        const state = usePlayerStore.getState();
        const { snapshot } = state;
        const { hasCdg } = useCdgStore.getState();

        if (!hasCdg || !snapshot?.is_playing || pendingRef.current) {
          return;
        }
        pendingRef.current = true;
        const positionMs = selectAudiencePreviewPositionMs(state);

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
      },
      (listener) =>
        usePlayerStore.subscribe((state, previousState) => {
          listener(
            selectAudiencePreviewPositionMs(state),
            selectAudiencePreviewPositionMs(previousState),
          );
        }),
    );

    return () => {
      stopSync();
    };
  }, [enabled]);
}
