import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { PlaybackStage } from "./PlaybackStage";

const { mockCdgState, mockPlayerState, mockLibraryState } = vi.hoisted(() => ({
  mockCdgState: { hasCdg: false },
  mockPlayerState: {
    snapshot: {
      song_id: "song-1",
    },
  },
  mockLibraryState: {
    songs: [
      {
        hash: "song-1",
        file_path: "media-g/song-1.mp3",
        cdg_path: "media-g/song-1.cdg",
        media_g_container: "paired" as const,
        title: "Song",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "mp3",
      },
    ],
  },
}));

vi.mock("@/stores/cdg-store", () => ({
  useCdgStore: (selector: (state: typeof mockCdgState) => unknown) =>
    selector(mockCdgState),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
}));

vi.mock("@/stores/library-store", () => ({
  useLibraryStore: (selector: (state: typeof mockLibraryState) => unknown) =>
    selector(mockLibraryState),
}));

vi.mock("@/components/Cdg/CdgCanvas", () => ({
  CdgCanvas: () => <div data-testid="cdg-canvas">CDG</div>,
}));

vi.mock("@/components/Lyrics/LyricsPanel", () => ({
  LyricsPanel: () => <div data-testid="lyrics-panel">Lyrics</div>,
}));

describe("PlaybackStage", () => {
  test("renders the CDG canvas when the current song metadata has CDG media", () => {
    const markup = renderToStaticMarkup(<PlaybackStage />);

    expect(markup).toContain("flex h-full w-full flex-1 overflow-hidden");
    expect(markup).toContain("cdg-canvas");
    expect(markup).not.toContain("lyrics-panel");
  });
});
