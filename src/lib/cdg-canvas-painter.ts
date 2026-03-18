/** CDG visible frame dimensions. */
export const CDG_WIDTH = 288;
export const CDG_HEIGHT = 192;

/**
 * Module-level canvas element reference. The CdgCanvas component registers its
 * canvas here so that the rAF loop can paint directly without going through
 * React/Zustand state updates. CDG can update many times per second, so pushing
 * every frame through React would add avoidable render churn.
 *
 * Each Tauri WebviewWindow runs its own JS context, so the module-level
 * variables are independent between the main window and the fullscreen window.
 */
let cdgCanvasEl: HTMLCanvasElement | null = null;
let cdgCanvasCtx: CanvasRenderingContext2D | null = null;
let lastFrameBytes: Uint8ClampedArray | null = null;

/**
 * PERF: Pre-allocated ImageData reused across frames. Creating a new
 * `ImageData` (221 KB) on every frame at 30fps produces ~6.5 MB/s of GC
 * pressure. Reusing one instance eliminates this entirely. Do not change
 * `drawFrame` to allocate a new `ImageData` per call.
 */
let reusableImageData: ImageData | null = null;

function ensureImageData(): ImageData {
  if (!reusableImageData) {
    reusableImageData = new ImageData(CDG_WIDTH, CDG_HEIGHT);
  }

  return reusableImageData;
}

function paintBytes(bytes: Uint8ClampedArray | Uint8Array): void {
  if (!cdgCanvasCtx || !cdgCanvasEl) return;

  const imageData = ensureImageData();
  imageData.data.set(bytes);
  cdgCanvasCtx.putImageData(imageData, 0, 0);
}

export function setCdgCanvas(canvas: HTMLCanvasElement | null): void {
  cdgCanvasEl = canvas;
  cdgCanvasCtx = canvas?.getContext("2d") ?? null;
  // Reset pre-allocated ImageData when canvas changes (new context).
  reusableImageData = null;

  if (lastFrameBytes) {
    paintBytes(lastFrameBytes);
  }
}

export function hasCdgCanvas(): boolean {
  return cdgCanvasEl !== null;
}

/**
 * Paint a raw RGBA frame (as `ArrayBuffer` from the Tauri IPC binary path)
 * directly onto the CDG canvas.
 *
 * PERF: This is the **performance-critical rendering path** for the main
 * window. The backend returns raw bytes via `tauri::ipc::Response` and the
 * IPC bridge delivers them as an `ArrayBuffer`. We wrap the buffer in a
 * `Uint8Array` view (O(1), no copy) and `.set()` it into the pre-allocated
 * `ImageData`. This avoids:
 *   1. Base64 decoding (`atob` + O(n) `charCodeAt` loop)
 *   2. Per-frame `ImageData` allocation (221 KB GC pressure)
 *
 * Do not revert to base64 string input or per-frame `new ImageData()` —
 * both were the primary CDG performance bottlenecks before this optimization.
 */
export function drawFrame(buffer: ArrayBuffer): void {
  lastFrameBytes = new Uint8ClampedArray(buffer);
  paintBytes(lastFrameBytes);
}

/**
 * Paint a base64-encoded RGBA frame onto the CDG canvas.
 *
 * Used exclusively by the **fullscreen window's event receiver path**, where
 * frames arrive as base64 strings through Tauri's JSON-serialized event
 * system (which cannot carry raw `ArrayBuffer` payloads). The main window
 * uses `drawFrame(ArrayBuffer)` instead for better performance.
 *
 * This path is intentionally separate from `drawFrame` — do not merge them.
 * The base64 overhead here is acceptable because the fullscreen window is a
 * secondary display; the main window's rendering path must remain binary.
 */
export function drawFrameFromBase64(base64Frame: string): void {
  const binary = atob(base64Frame);
  const bytes = new Uint8ClampedArray(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }

  lastFrameBytes = bytes;
  paintBytes(bytes);
}

export function clearFrame(): void {
  lastFrameBytes = null;
  cdgCanvasCtx?.clearRect(0, 0, CDG_WIDTH, CDG_HEIGHT);
}
