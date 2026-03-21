import { useEffect, useMemo } from "react";
import type { CoverArtBytes } from "@/types/ipc";

interface CoverArtCacheEntry {
  refs: number;
  url: string;
}

const coverArtUrlCache = new Map<string, CoverArtCacheEntry>();

function ensureCoverArtBytes(input: CoverArtBytes): Uint8Array<ArrayBuffer> | null {
  if (!input) {
    return null;
  }

  // Tauri IPC usually preserves `Vec<u8>` as a JSON array, but binary values
  // can also arrive as ArrayBuffer / typed-array views depending on the bridge
  // path. Cover art rendering must normalize those runtime shapes first.
  if (input instanceof ArrayBuffer) {
    return new Uint8Array(input);
  }

  if (ArrayBuffer.isView(input)) {
    return Uint8Array.from(
      new Uint8Array(input.buffer, input.byteOffset, input.byteLength),
    );
  }

  if (Array.isArray(input)) {
    return Uint8Array.from(input);
  }

  return null;
}

export function detectCoverArtMime(bytes: CoverArtBytes): string {
  const normalizedBytes = ensureCoverArtBytes(bytes);
  if (!normalizedBytes || normalizedBytes.byteLength === 0) {
    return "image/jpeg";
  }

  if (normalizedBytes[0] === 0xff && normalizedBytes[1] === 0xd8) {
    return "image/jpeg";
  }
  if (
    normalizedBytes[0] === 0x89 &&
    normalizedBytes[1] === 0x50 &&
    normalizedBytes[2] === 0x4e &&
    normalizedBytes[3] === 0x47
  ) {
    return "image/png";
  }
  if (
    normalizedBytes[0] === 0x47 &&
    normalizedBytes[1] === 0x49 &&
    normalizedBytes[2] === 0x46
  ) {
    return "image/gif";
  }
  if (
    normalizedBytes[0] === 0x52 &&
    normalizedBytes[1] === 0x49 &&
    normalizedBytes[2] === 0x46
  ) {
    return "image/webp";
  }
  return "image/jpeg";
}

export function retainCoverArtUrl(
  songHash: string,
  bytes: CoverArtBytes,
): string | null {
  const normalizedBytes = ensureCoverArtBytes(bytes);

  if (
    !normalizedBytes ||
    normalizedBytes.byteLength === 0 ||
    typeof URL === "undefined" ||
    typeof URL.createObjectURL !== "function"
  ) {
    return null;
  }

  const cached = coverArtUrlCache.get(songHash);
  if (cached) {
    cached.refs += 1;
    return cached.url;
  }

  const url = URL.createObjectURL(
    new Blob([normalizedBytes], { type: detectCoverArtMime(normalizedBytes) }),
  );

  coverArtUrlCache.set(songHash, {
    refs: 1,
    url,
  });

  return url;
}

export function releaseCoverArtUrl(songHash: string): void {
  const cached = coverArtUrlCache.get(songHash);
  if (!cached) {
    return;
  }

  cached.refs -= 1;
  if (cached.refs > 0) {
    return;
  }

  if (typeof URL !== "undefined" && typeof URL.revokeObjectURL === "function") {
    URL.revokeObjectURL(cached.url);
  }
  coverArtUrlCache.delete(songHash);
}

export function invalidateCoverArtUrl(songHash: string): void {
  const cached = coverArtUrlCache.get(songHash);
  if (!cached) {
    return;
  }

  if (typeof URL !== "undefined" && typeof URL.revokeObjectURL === "function") {
    URL.revokeObjectURL(cached.url);
  }

  coverArtUrlCache.delete(songHash);
}

export function resetCoverArtCacheForTests(): void {
  for (const [songHash, entry] of coverArtUrlCache) {
    if (
      typeof URL !== "undefined" &&
      typeof URL.revokeObjectURL === "function"
    ) {
      URL.revokeObjectURL(entry.url);
    }
    coverArtUrlCache.delete(songHash);
  }
}

export function useCoverArtUrl(
  songHash: string,
  bytes: CoverArtBytes,
): string | null {
  const url = useMemo(
    () => retainCoverArtUrl(songHash, bytes),
    [songHash, bytes],
  );

  useEffect(() => {
    if (!url) {
      return;
    }

    return () => {
      releaseCoverArtUrl(songHash);
    };
  }, [songHash, url]);

  return url;
}
