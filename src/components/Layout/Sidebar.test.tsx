import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { Sidebar } from "./Sidebar";
import type { Song } from "@/types/ipc";

const { mockLibraryState, mockSettingsState } = vi.hoisted(() => ({
  mockLibraryState: {
    songs: [
      {
        hash: "song-1",
        file_path: "media-g/song-1.mp3",
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
    filter: "all" as const,
    setFilter: vi.fn(),
    separationStatuses: {},
    batchSeparation: null,
  },
  mockSettingsState: {
    hideBatchSeparate: false,
    stemMode: "two_stem",
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/stores/library-store", () => ({
  useLibraryStore: (selector: (state: typeof mockLibraryState) => unknown) =>
    selector(mockLibraryState),
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

vi.mock("@/components/Library/SearchBox", () => ({
  SearchBox: () => <div data-search-visual-variant="mock">search</div>,
}));

vi.mock("@/components/Library/SongList", () => ({
  SongList: () => <div data-song-list-visual-variant="mock">songs</div>,
}));

vi.mock("@/components/Library/ImportButton", () => ({
  ImportButton: ({ children }: { children: React.ReactNode }) => children,
}));

vi.mock("@/components/Settings/ConfirmationDialog", () => ({
  ConfirmationDialog: () => <div>confirm</div>,
}));

vi.mock("@/lib/tauri", () => ({
  batchSeparate: vi.fn(),
}));

vi.mock("@/lib/errors", () => ({
  notifyError: vi.fn(),
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({ children }: { children: React.ReactNode }) => children,
}));

vi.mock("@/lib/app-shortcuts", () => ({
  APP_SHORTCUTS: {},
  getShortcutDisplay: () => "",
}));

describe("Sidebar", () => {
  test("does not render a duplicate import icon beside the local music heading", () => {
    const markup = renderToStaticMarkup(<Sidebar />);

    expect(markup).not.toContain("lucide-cloud-upload");
  });

  test("hides separate-all controls when the library has only media-g songs", () => {
    const markup = renderToStaticMarkup(<Sidebar />);

    expect(markup).not.toContain("sidebar.separateAll");
    expect(markup).not.toContain("sidebar.upgradeAll");
  });

  test("hides separate-all controls when every plain-audio song is instrumental", () => {
    mockLibraryState.songs = [
      {
        hash: "song-2",
        file_path: "music/song-2.mp3",
        cdg_path: null,
        media_g_container: null,
        instrumental: true,
        title: "Instrumental",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "mp3",
      },
    ] as Song[];

    const markup = renderToStaticMarkup(<Sidebar />);

    expect(markup).not.toContain("sidebar.separateAll");
    expect(markup).not.toContain("sidebar.upgradeAll");
  });

  test("uses the unified sidebar surface and composition markers", () => {
    const markup = renderToStaticMarkup(<Sidebar />);

    expect(markup).toContain('data-sidebar-visual-variant="unified"');
    expect(markup).toContain('data-search-visual-variant="mock"');
    expect(markup).toContain('data-song-list-visual-variant="mock"');
  });

  test("renders batch actions with shared sidebar control tokens", () => {
    mockLibraryState.songs = [
      {
        hash: "song-3",
        file_path: "music/song-3.mp3",
        cdg_path: null,
        media_g_container: null,
        instrumental: false,
        title: "Song 3",
        artist: null,
        album: null,
        duration_ms: 1000,
        cover_art: null,
        imported_at: 0,
        original_ext: "mp3",
      },
    ] as Song[];

    const markup = renderToStaticMarkup(<Sidebar />);

    expect(markup).toContain("bg-[var(--sidebar-control-bg)]");
    expect(markup).toContain("border-[var(--sidebar-control-border)]");
    expect(markup).toContain("hover:bg-[var(--sidebar-row-overlay-bg)]");
    expect(markup).toContain("hover:border-[var(--sidebar-control-border)]");
    expect(markup).not.toContain(
      "hover:border-[var(--sidebar-row-selected-border)]",
    );

    mockLibraryState.songs = [
      {
        hash: "song-1",
        file_path: "media-g/song-1.mp3",
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
    ] as Song[];
  });
});
