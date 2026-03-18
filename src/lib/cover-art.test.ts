import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import {
  detectCoverArtMime,
  invalidateCoverArtUrl,
  releaseCoverArtUrl,
  resetCoverArtCacheForTests,
  retainCoverArtUrl,
} from "./cover-art";

describe("cover-art", () => {
  beforeEach(() => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn((blob: Blob) => `blob:${blob.type}`),
      revokeObjectURL: vi.fn(),
    });
  });

  afterEach(() => {
    resetCoverArtCacheForTests();
    vi.unstubAllGlobals();
  });

  test("detects common cover art mime types", () => {
    expect(detectCoverArtMime([0xff, 0xd8, 0x00])).toBe("image/jpeg");
    expect(detectCoverArtMime([0x89, 0x50, 0x4e, 0x47])).toBe("image/png");
    expect(detectCoverArtMime([0x47, 0x49, 0x46])).toBe("image/gif");
    expect(detectCoverArtMime([0x52, 0x49, 0x46, 0x46])).toBe("image/webp");
  });

  test("reuses cached object urls per song hash and revokes after the final release", () => {
    const jpegBytes = [0xff, 0xd8, 0x00];

    const first = retainCoverArtUrl("song-1", jpegBytes);
    const second = retainCoverArtUrl("song-1", jpegBytes);

    expect(first).toBe("blob:image/jpeg");
    expect(second).toBe(first);
    expect(URL.createObjectURL).toHaveBeenCalledTimes(1);

    releaseCoverArtUrl("song-1");
    expect(URL.revokeObjectURL).not.toHaveBeenCalled();

    releaseCoverArtUrl("song-1");
    expect(URL.revokeObjectURL).toHaveBeenCalledWith("blob:image/jpeg");
  });

  test("invalidates a cached object url so a refreshed cover can replace it immediately", () => {
    const first = retainCoverArtUrl("song-1", [0xff, 0xd8, 0x00]);

    expect(first).toBe("blob:image/jpeg");
    expect(URL.createObjectURL).toHaveBeenCalledTimes(1);

    invalidateCoverArtUrl("song-1");

    expect(URL.revokeObjectURL).toHaveBeenCalledWith("blob:image/jpeg");

    const second = retainCoverArtUrl("song-1", [0x89, 0x50, 0x4e, 0x47]);

    expect(second).toBe("blob:image/png");
    expect(URL.createObjectURL).toHaveBeenCalledTimes(2);
  });
});
