import { renderToStaticMarkup } from "react-dom/server";
import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { resetCoverArtCacheForTests } from "@/lib/cover-art";
import { CoverArtThumbnail } from "./CoverArtThumbnail";

describe("CoverArtThumbnail", () => {
  beforeEach(() => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn(() => "blob:cover"),
      revokeObjectURL: vi.fn(),
    });
  });

  afterEach(() => {
    resetCoverArtCacheForTests();
    vi.unstubAllGlobals();
  });

  test("renders eager cover art images for local database artwork", () => {
    const markup = renderToStaticMarkup(
      <CoverArtThumbnail
        songHash="song-1"
        coverArt={[0xff, 0xd8, 0x00]}
        alt="Bistro cover art"
        className="h-12 w-12"
      />,
    );

    expect(markup).toContain("<img");
    expect(markup).toContain('src="blob:cover"');
    expect(markup).not.toContain('loading="lazy"');
    expect(markup).not.toContain('decoding="async"');
  });

  test("renders the placeholder when no cover art url can be produced", () => {
    const markup = renderToStaticMarkup(
      <CoverArtThumbnail
        songHash="song-2"
        coverArt={null}
        alt="Missing cover art"
        className="h-12 w-12"
      />,
    );

    expect(markup).not.toContain("<img");
    expect(markup).toContain('aria-hidden="true"');
  });
});
