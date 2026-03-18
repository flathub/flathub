import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
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
        title: "22",
        artist: "Taylor Swift",
        album: null,
        duration_ms: 242000,
        cover_art: [0xff, 0xd8, 0x00],
        imported_at: 0,
        original_ext: "mp3",
      },
    ],
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

    vi.unstubAllGlobals();
  });
});
