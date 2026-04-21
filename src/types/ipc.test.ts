import { describe, expect, test } from "vitest";
import type { RegisteredLibrary, Song } from "./ipc";

describe("ipc type shapes", () => {
  test("accepts remote songs without a local file path", () => {
    const song: Song = {
      hash: "song-remote",
      file_path: null,
      audio_source_kind: "original_remote",
      cdg_path: null,
      media_g_container: null,
      instrumental: false,
      title: "Remote Song",
      artist: "Artist",
      album: null,
      duration_ms: 123000,
      cover_art: null,
      imported_at: 0,
      original_ext: "mp3",
    };

    expect(song.audio_source_kind).toBe("original_remote");
  });

  test("accepts remote library registrations with provider metadata", () => {
    const library: RegisteredLibrary = {
      id: "remote:drive",
      kind: "remote",
      display_name: "Drive Library",
      provider: "webdav",
      remote_root_locator: "drive-root",
      remote_path_display: "OpenKara / Team Karaoke",
      account_id: "acct-1",
      cached_db_path: null,
      remote_revision: null,
      bound_local_library_id: null,
    };

    expect(library.provider).toBe("webdav");
  });
});
