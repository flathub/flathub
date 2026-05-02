import { invoke } from "@tauri-apps/api/core";

/**
 * Returns a raw RGBA frame (288x192) as an `ArrayBuffer` for the given
 * playback position. An empty buffer (`byteLength === 0`) means no CDG is
 * active or the frame hasn't changed.
 *
 * PERF: The backend returns raw bytes via `tauri::ipc::Response`, which the
 * IPC bridge delivers as an `ArrayBuffer` - no base64 encoding/decoding is
 * involved. This is a deliberate performance choice: base64 inflates the
 * payload by ~33% and requires an expensive O(n) decode loop on the main
 * thread. Do not change the return type to `string` without benchmarking.
 */
export function getCdgFrame(ms: number): Promise<ArrayBuffer> {
  return invoke<ArrayBuffer>("get_cdg_frame", {
    positionMs: Math.round(ms),
  });
}
