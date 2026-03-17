import { useEffect, useRef } from "react";
import { usePlayerStore } from "@/stores/player-store";
import { useCdgStore } from "@/stores/cdg-store";
import * as api from "@/lib/tauri";

/** CDG visible frame dimensions. */
const CDG_WIDTH = 288;
const CDG_HEIGHT = 192;

/** Minimum interval between IPC calls (~15 fps). */
const MIN_INTERVAL_MS = 66;

/**
 * Module-level canvas element reference. The CdgCanvas component registers its
 * canvas here so that the rAF loop can paint directly without going through
 * React/Zustand state updates. CDG can update many times per second, so pushing
 * every frame through React would add avoidable render churn to fullscreen playback.
 */
let cdgCanvasEl: HTMLCanvasElement | null = null;
let cdgCanvasCtx: CanvasRenderingContext2D | null = null;

export function setCdgCanvas(canvas: HTMLCanvasElement | null): void {
  cdgCanvasEl = canvas;
  cdgCanvasCtx = canvas?.getContext("2d") ?? null;
}

function drawFrame(base64Frame: string): void {
  if (!cdgCanvasCtx || !cdgCanvasEl) return;

  const binary = atob(base64Frame);
  const bytes = new Uint8ClampedArray(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }

  cdgCanvasCtx.putImageData(new ImageData(bytes, CDG_WIDTH, CDG_HEIGHT), 0, 0);
}

function clearFrame(): void {
  cdgCanvasCtx?.clearRect(0, 0, CDG_WIDTH, CDG_HEIGHT);
}

export function useCdgSync(): void {
  const songId = usePlayerStore((s) => s.snapshot?.song_id ?? null);
  const currentCdgSongId = useCdgStore((s) => s.songId);
  const setSong = useCdgStore((s) => s.setSong);
  const clear = useCdgStore((s) => s.clear);
  const rafRef = useRef<number>(0);
  const lastCallRef = useRef(0);
  const pendingRef = useRef(false);

  useEffect(() => {
    if (!songId) {
      clear();
      clearFrame();
      return;
    }

    let cancelled = false;
    const probePositionMs = usePlayerStore.getState().positionMs;

    if (currentCdgSongId !== songId) {
      // Clear immediately on song change so a previous song's CDG frame does not
      // linger while we asynchronously probe whether the new track has graphics.
      setSong(songId, false);
      clearFrame();
    }

    api
      .getCdgFrame(probePositionMs)
      .then((base64Frame) => {
        if (cancelled) return;

        if (base64Frame) {
          setSong(songId, true);
          drawFrame(base64Frame);
          lastCallRef.current = 0;
          return;
        }

        setSong(songId, false);
        clearFrame();
      })
      .catch(() => {
        if (cancelled) return;
        setSong(songId, false);
        clearFrame();
      });

    return () => {
      cancelled = true;
    };
  }, [clear, currentCdgSongId, setSong, songId]);

  useEffect(() => {
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

      // Keep the hot frame path out of React state: the store only tracks whether
      // a song has CDG, while pixel updates go straight to the canvas.
      api
        .getCdgFrame(positionMs)
        .then((base64Frame) => {
          if (base64Frame) drawFrame(base64Frame);
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
  }, []);
}
