import { describe, expect, test } from "vitest";
import { songHasCdgMedia } from "./song-media";

describe("songHasCdgMedia", () => {
  test("returns true for paired CDG songs", () => {
    expect(
      songHasCdgMedia({
        hash: "song-1",
        file_path: "media-g/song-1.mp3",
        cdg_path: "media-g/song-1.cdg",
        media_g_container: "paired",
        title: "Song",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "mp3",
      }),
    ).toBe(true);
  });

  test("returns true for media+g zip songs", () => {
    expect(
      songHasCdgMedia({
        hash: "song-2",
        file_path: "media-g/song-2.zip",
        cdg_path: null,
        media_g_container: "zip",
        title: "Song",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "zip",
      }),
    ).toBe(true);
  });
});
