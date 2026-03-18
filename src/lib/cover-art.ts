import { useEffect, useMemo } from "react";

interface CoverArtCacheEntry {
  refs: number;
  url: string;
}

const coverArtUrlCache = new Map<string, CoverArtCacheEntry>();

export function detectCoverArtMime(bytes: number[]): string {
  if (bytes[0] === 0xff && bytes[1] === 0xd8) return "image/jpeg";
  if (
    bytes[0] === 0x89 &&
    bytes[1] === 0x50 &&
    bytes[2] === 0x4e &&
    bytes[3] === 0x47
  ) {
    return "image/png";
  }
  if (bytes[0] === 0x47 && bytes[1] === 0x49 && bytes[2] === 0x46) {
    return "image/gif";
  }
  if (bytes[0] === 0x52 && bytes[1] === 0x49 && bytes[2] === 0x46) {
    return "image/webp";
  }
  return "image/jpeg";
}

export function retainCoverArtUrl(
  songHash: string,
  bytes: number[] | null,
): string | null {
  if (
    !bytes ||
    bytes.length === 0 ||
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
    new Blob([new Uint8Array(bytes)], { type: detectCoverArtMime(bytes) }),
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
  bytes: number[] | null,
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
