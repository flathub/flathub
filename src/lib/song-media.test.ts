import { describe, expect, test } from "vitest";
import {
  songCanBeSeparated,
  songHasCdgMedia,
  songSupportsInstrumentalFlag,
} from "./song-media";

describe("songHasCdgMedia", () => {
  test("returns true for paired CDG songs", () => {
    expect(
      songHasCdgMedia({
        hash: "song-1",
        file_path: "media-g/song-1.mp3",
        audio_source_kind: "original",
        cdg_path: "media-g/song-1.cdg",
        media_g_container: "paired",
        instrumental: false,
        language: null,
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
        audio_source_kind: "original",
        cdg_path: null,
        media_g_container: "zip",
        instrumental: false,
        language: null,
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

  test("returns true for imported songs without Media+G graphics", () => {
    expect(
      songSupportsInstrumentalFlag({
        hash: "song-3",
        file_path: "music/song.mp3",
        audio_source_kind: "original",
        cdg_path: null,
        media_g_container: null,
        instrumental: false,
        language: null,
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

  test("returns false for songs already marked instrumental", () => {
    expect(
      songCanBeSeparated({
        hash: "song-4",
        file_path: "music/song.mp3",
        audio_source_kind: "original",
        cdg_path: null,
        media_g_container: null,
        instrumental: true,
        language: null,
        title: "Song",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "mp3",
      }),
    ).toBe(false);
  });

  test("returns false for missing songs", () => {
    expect(songCanBeSeparated(undefined)).toBe(false);
    expect(songCanBeSeparated(null)).toBe(false);
  });
});
