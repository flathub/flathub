import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import type { Song } from "@/types/ipc";
import { NowPlayingInfo } from "./NowPlayingInfo";

const { mockPlayerState, mockLibraryState } = vi.hoisted(() => ({
  mockPlayerState: {
    snapshot: {
      song_id: "song-1",
    },
  },
  mockLibraryState: {
    songs: [
      {
        hash: "song-1",
        file_path: "Taylor Swift/22.mp3",
        cdg_path: null,
        media_g_container: null,
        instrumental: false,
        title: "22",
        artist: "Taylor Swift",
        album: null,
        duration_ms: 242000,
        cover_art: [0xff, 0xd8, 0x00],
        imported_at: 0,
        original_ext: "mp3",
      },
    ] as Song[],
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
}));

vi.mock("@/stores/library-store", () => ({
  useLibraryStore: (selector: (state: typeof mockLibraryState) => unknown) =>
    selector(mockLibraryState),
}));

describe("NowPlayingInfo", () => {
  test("renders cover art alongside the current song metadata", () => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn(() => "blob:cover"),
      revokeObjectURL: vi.fn(),
    });

    const markup = renderToStaticMarkup(<NowPlayingInfo />);

    expect(markup).toContain("<img");
    expect(markup).toContain('src="blob:cover"');
    expect(markup).toContain(">22<");
    expect(markup).toContain(">Taylor Swift<");
    expect(markup).not.toContain('loading="lazy"');
    expect(markup).not.toContain('decoding="async"');

    vi.unstubAllGlobals();
  });

  test("renders cover art when bytes arrive as ArrayBuffer", () => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn(() => "blob:cover"),
      revokeObjectURL: vi.fn(),
    });

    mockLibraryState.songs[0].cover_art = new Uint8Array([
      0xff, 0xd8, 0x00,
    ]).buffer;

    const markup = renderToStaticMarkup(<NowPlayingInfo />);

    expect(markup).toContain("<img");
    expect(markup).toContain('src="blob:cover"');
    expect(markup).not.toContain('loading="lazy"');
    expect(markup).not.toContain('decoding="async"');

    vi.unstubAllGlobals();
  });

  test("hides the artist metadata in the tight density", () => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn(() => "blob:cover"),
      revokeObjectURL: vi.fn(),
    });

    const markup = renderToStaticMarkup(<NowPlayingInfo density="tight" />);

    expect(markup).toContain(">22<");
    expect(markup).not.toContain(">Taylor Swift<");

    vi.unstubAllGlobals();
  });
});
