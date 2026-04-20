import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { SongListItem } from "./SongListItem";
import { buildSongListContextMenuItems } from "./song-list-item-menu";

const {
  mockLibraryState,
  mockPlayerState,
  mockLyricsState,
  mockSettingsState,
} = vi.hoisted(() => ({
  mockLibraryState: {
    selectedSongIds: new Set<string>(),
    selectSong: vi.fn(),
    separationStatuses: {},
    uploadStatuses: {},
    songs: [],
    loadLibrary: vi.fn(),
    lastClickedSongId: null,
    extractEmbeddedCoverArt: vi.fn(),
    setSongsInstrumental: vi.fn(),
  },
  mockPlayerState: {
    snapshot: null,
    playSong: vi.fn(),
    loadState: vi.fn(),
  },
  mockLyricsState: {
    songId: null,
    clear: vi.fn(),
  },
  mockSettingsState: {
    close: vi.fn(),
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string, vars?: Record<string, string | number>) =>
      vars?.title ? `${key}:${vars.title}` : key,
  }),
}));

vi.mock("@/stores/library-store", () => ({
  useLibraryStore: Object.assign(
    (selector: (state: typeof mockLibraryState) => unknown) =>
      selector(mockLibraryState),
    {
      setState: vi.fn(),
    },
  ),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: Object.assign(
    (selector: (state: typeof mockPlayerState) => unknown) =>
      selector(mockPlayerState),
    {
      getState: () => mockPlayerState,
    },
  ),
}));

vi.mock("@/stores/lyrics-store", () => ({
  useLyricsStore: {
    getState: () => mockLyricsState,
  },
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

vi.mock("@/stores/queue-store", () => ({
  useQueueStore: {
    getState: () => ({
      addToQueue: vi.fn(),
      removeSongIds: vi.fn(),
    }),
  },
}));

vi.mock("@/lib/tauri", () => ({
  separate: vi.fn(),
  deleteSongs: vi.fn(),
  batchSeparate: vi.fn(),
  extractEmbeddedLyrics: vi.fn(),
  fetchLyricsOnline: vi.fn(() => Promise.resolve({ lines: [] })),
}));

vi.mock("@/lib/errors", () => ({
  notifyError: vi.fn(),
}));

vi.mock("./ContextMenu", () => ({
  ContextMenu: ({
    items,
  }: {
    items: Array<{
      label: string;
    }>;
  }) => <div>{items.map((item) => item.label).join(" | ")}</div>,
}));

vi.mock("../Settings/ConfirmationDialog", () => ({
  ConfirmationDialog: () => <div>confirm dialog</div>,
}));

vi.mock("./SongEditDialog", () => ({
  SongEditDialog: () => <div>edit dialog</div>,
}));

vi.mock("./SongPropertiesDialog", () => ({
  SongPropertiesDialog: () => <div>properties dialog</div>,
}));

describe("SongListItem", () => {
  test("renders media-g badges and duration in the trailing metadata slot", () => {
    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-cdg",
          file_path: "Taylor Swift/22.mp3",
          cdg_path: "Taylor Swift/22.cdg",
          media_g_container: "paired",
          instrumental: false,
          title: "22 [Z Karaoke]",
          artist: "Taylor Swift",
          album: null,
          duration_ms: 246000,
          cover_art: null,
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-cdg"]}
      />,
    );

    expect(markup).toContain(">CDG<");
    expect(markup).toContain("4:06");
    expect(markup).not.toContain("2</span>4:06");
  });

  test("renders a compact cover art thumbnail when cover art is available", () => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn(() => "blob:cover"),
      revokeObjectURL: vi.fn(),
    });

    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-1",
          file_path: "Brent Faiyaz/Loose Change.mp3",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          title: "LOOSE CHANGE",
          artist: "Brent Faiyaz",
          album: null,
          duration_ms: 226000,
          cover_art: [0xff, 0xd8, 0x00],
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-1"]}
      />,
    );

    expect(markup).toContain("<img");
    expect(markup).toContain('src="blob:cover"');
    expect(markup).toContain("LOOSE CHANGE");
    expect(markup).not.toContain('loading="lazy"');
    expect(markup).not.toContain('decoding="async"');

    vi.unstubAllGlobals();
  });

  test("renders unified row hooks for every song row", () => {
    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-native",
          file_path: "Fuji Kaze/Hachiko.mp3",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          title: "Hachiko",
          artist: "Fuji Kaze",
          album: null,
          duration_ms: 270000,
          cover_art: null,
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-native"]}
      />,
    );

    expect(markup).toContain('data-song-list-item-variant="unified"');
    expect(markup).toContain('data-native-overlay-surface="song-row"');
    expect(markup).toContain("hover:bg-[var(--sidebar-row-overlay-bg)]");
  });

  test("renders selected rows and actions as overlay surfaces", () => {
    mockLibraryState.selectedSongIds = new Set(["song-native-selected"]);

    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-native-selected",
          file_path: "Rina Sawayama/Hold The Girl.mp3",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          title: "Hold The Girl",
          artist: "Rina Sawayama",
          album: null,
          duration_ms: 240000,
          cover_art: null,
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-native-selected"]}
      />,
    );

    expect(markup).toContain("bg-[var(--sidebar-row-selected-bg)]");
    expect(markup).toContain("border-[var(--sidebar-row-selected-border)]");
    expect(markup).toContain('data-native-overlay-surface="song-action"');
    expect(markup).toContain("bg-[var(--sidebar-control-bg)]");
    expect(markup).toContain("border-[var(--sidebar-control-border)]");

    mockLibraryState.selectedSongIds = new Set();
  });

  test("renders badges as overlay surfaces instead of opaque fills", () => {
    mockLibraryState.selectedSongIds = new Set();
    mockLibraryState.separationStatuses = {
      "song-native-badges": {
        state: "completed",
        drums_path: "drums.ogg",
      },
    };

    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-native-badges",
          file_path: "Rina Sawayama/Hold The Girl.mp3",
          cdg_path: null,
          media_g_container: "paired",
          instrumental: false,
          title: "Hold The Girl",
          artist: "Rina Sawayama",
          album: null,
          duration_ms: 240000,
          cover_art: null,
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-native-badges"]}
      />,
    );

    expect(markup).toContain("bg-[var(--sidebar-row-overlay-bg)]");
    expect(markup).not.toContain("bg-[var(--color-hover)]");

    mockLibraryState.separationStatuses = {};
  });

  test("renders shared progress bars for running separation and upload tasks", () => {
    mockLibraryState.selectedSongIds = new Set();
    mockLibraryState.separationStatuses = {
      "song-native-progress": {
        song_id: "song-native-progress",
        state: "running",
        percent: 55,
        cache_hit: false,
        vocals_path: null,
        accomp_path: null,
        drums_path: null,
        bass_path: null,
        other_path: null,
        model_variant: null,
        error: null,
      },
    };
    mockLibraryState.uploadStatuses = {
      "song-native-progress": {
        song_id: "song-native-progress",
        state: "running",
        percent: 88,
        remote_library_id: null,
        detail: null,
        error: null,
      },
    };

    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-native-progress",
          file_path: "Rina Sawayama/Hold The Girl.mp3",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          title: "Hold The Girl",
          artist: "Rina Sawayama",
          album: null,
          duration_ms: 240000,
          cover_art: null,
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-native-progress"]}
      />,
    );

    expect(markup).toContain("progress.separating:Hold The Girl");
    expect(markup).toContain("progress.uploadingToRemote:Hold The Girl");
    expect(markup).toContain("h-1.5 w-full overflow-hidden rounded-full");

    mockLibraryState.separationStatuses = {};
    mockLibraryState.uploadStatuses = {};
  });

  test("renders a compact cover art thumbnail when cover art arrives as Uint8Array", () => {
    vi.stubGlobal("URL", {
      createObjectURL: vi.fn(() => "blob:cover"),
      revokeObjectURL: vi.fn(),
    });

    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-typed-array",
          file_path: "Madvillain/Bistro.m4a",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          title: "Bistro",
          artist: "Madvillain",
          album: null,
          duration_ms: 67000,
          cover_art: new Uint8Array([0xff, 0xd8, 0x00]),
          imported_at: 0,
          original_ext: "m4a",
        }}
        orderedHashes={["song-typed-array"]}
      />,
    );

    expect(markup).toContain("<img");
    expect(markup).toContain('src="blob:cover"');
    expect(markup).not.toContain('loading="lazy"');
    expect(markup).not.toContain('decoding="async"');

    vi.unstubAllGlobals();
  });

  test("shows extract embedded cover art in the single-song context menu", () => {
    const labels = buildSongListContextMenuItems({
      t: (key: string) => key,
      isMultiSelected: false,
      selectedCount: 1,
      selectedSongIds: ["song-1"],
      selectedHasSeparableSongs: true,
      selectedCanToggleInstrumentalSongs: true,
      selectedInstrumentalState: "unchecked",
      supportsEmbeddedLyrics: true,
      queueAllSelected: vi.fn(),
      separateAllSelected: vi.fn(),
      toggleSelectedInstrumental: vi.fn(),
      extractSelectedEmbeddedCoverArt: vi.fn(),
      deleteSelected: vi.fn(),
      playNow: vi.fn(),
      playNext: vi.fn(),
      addToQueue: vi.fn(),
      extractEmbeddedCoverArt: vi.fn(),
      extractEmbeddedLyrics: vi.fn(),
      fetchLyricsOnline: vi.fn(),
      editInfo: vi.fn(),
      openProperties: vi.fn(),
      deleteSong: vi.fn(),
    }).map((item) => item.label);

    expect(labels).toContain("library.extractEmbeddedCoverArt");
    expect(labels).toContain("library.extractEmbeddedLyrics");
  });

  test("shows multi-select embedded cover art extraction in the selected context menu", () => {
    const items = buildSongListContextMenuItems({
      t: (key: string, vars?: { count?: number }) =>
        vars?.count ? `${key}:${vars.count}` : key,
      isMultiSelected: true,
      selectedCount: 2,
      selectedSongIds: ["song-1", "song-2"],
      selectedHasSeparableSongs: true,
      selectedCanToggleInstrumentalSongs: true,
      selectedInstrumentalState: "mixed",
      supportsEmbeddedLyrics: false,
      queueAllSelected: vi.fn(),
      separateAllSelected: vi.fn(),
      toggleSelectedInstrumental: vi.fn(),
      extractSelectedEmbeddedCoverArt: vi.fn(),
      deleteSelected: vi.fn(),
      playNow: vi.fn(),
      playNext: vi.fn(),
      addToQueue: vi.fn(),
      extractEmbeddedCoverArt: vi.fn(),
      extractEmbeddedLyrics: vi.fn(),
      fetchLyricsOnline: vi.fn(),
      editInfo: vi.fn(),
      openProperties: vi.fn(),
      deleteSong: vi.fn(),
    });

    const labels = items.map((item) => item.label);

    expect(labels).toContain("library.markInstrumentalSelected:2");
    expect(labels).toContain("library.extractEmbeddedCoverArtSelected:2");
    expect(labels).not.toContain("library.extractEmbeddedLyrics");
    expect(
      items.find((item) => item.label === "library.markInstrumentalSelected:2")
        ?.indicator,
    ).toBe("mixed");
  });

  test("shows a checked instrumental toggle when every selected song is instrumental", () => {
    const items = buildSongListContextMenuItems({
      t: (key: string, vars?: { count?: number }) =>
        vars?.count ? `${key}:${vars.count}` : key,
      isMultiSelected: true,
      selectedCount: 2,
      selectedSongIds: ["song-1", "song-2"],
      selectedHasSeparableSongs: false,
      selectedCanToggleInstrumentalSongs: true,
      selectedInstrumentalState: "checked",
      supportsEmbeddedLyrics: false,
      queueAllSelected: vi.fn(),
      separateAllSelected: vi.fn(),
      toggleSelectedInstrumental: vi.fn(),
      extractSelectedEmbeddedCoverArt: vi.fn(),
      deleteSelected: vi.fn(),
      playNow: vi.fn(),
      playNext: vi.fn(),
      addToQueue: vi.fn(),
      extractEmbeddedCoverArt: vi.fn(),
      extractEmbeddedLyrics: vi.fn(),
      fetchLyricsOnline: vi.fn(),
      editInfo: vi.fn(),
      openProperties: vi.fn(),
      deleteSong: vi.fn(),
    });

    expect(
      items.find((item) => item.label === "library.markInstrumentalSelected:2")
        ?.indicator,
    ).toBe("checked");
  });

  test("does not render a separate button for instrumental songs", () => {
    const markup = renderToStaticMarkup(
      <SongListItem
        song={{
          hash: "song-instrumental",
          file_path: "Artist/Official Instrumental.mp3",
          cdg_path: null,
          media_g_container: null,
          instrumental: true,
          title: "Official Instrumental",
          artist: "Artist",
          album: null,
          duration_ms: 180000,
          cover_art: null,
          imported_at: 0,
          original_ext: "mp3",
        }}
        orderedHashes={["song-instrumental"]}
      />,
    );

    expect(markup).not.toContain("library.separate");
  });
});
