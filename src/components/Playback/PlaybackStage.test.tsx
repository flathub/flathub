import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { PlaybackStage } from "./PlaybackStage";
import type { Song } from "@/types/ipc";

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
        audio_source_kind: "original",
        cdg_path: "media-g/song-1.cdg",
        media_g_container: "paired" as const,
        instrumental: false,
        title: "Song",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "mp3",
      },
    ] as Song[],
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

vi.mock("@/lib/cover-art", () => ({
  useCoverArtUrl: () => "blob:stage-cover",
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

  test("applies a bottom inset for audience-safe overlays", () => {
    const markup = renderToStaticMarkup(
      <PlaybackStage presentation="audience" bottomInsetPx={144} />,
    );

    expect(markup).toContain('style="padding-bottom:144px"');
  });

  test("renders a cover-art ambience backdrop for standard lyric stages without CDG", () => {
    mockCdgState.hasCdg = false;
    mockLibraryState.songs = [
      {
        hash: "song-2",
        file_path: "Fuji Kaze/Hachiko.mp3",
        audio_source_kind: "original",
        cdg_path: null,
        media_g_container: null,
        instrumental: false,
        title: "Hachiko",
        artist: "Fuji Kaze",
        album: null,
        duration_ms: 270000,
        cover_art: [0xff, 0xd8, 0x00],
        imported_at: 0,
        original_ext: "mp3",
      },
    ] as Song[];
    mockPlayerState.snapshot = { song_id: "song-2" };

    const markup = renderToStaticMarkup(<PlaybackStage />);

    expect(markup).toContain('data-stage-visual-variant="ambience"');
    expect(markup).toContain('data-native-stage-backdrop="true"');
    expect(markup).toContain("blob:stage-cover");
    expect(markup).toContain("lyrics-panel");
  });
});
