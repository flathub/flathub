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
  SearchBox: ({ variant }: { variant?: string }) => (
    <div data-search-variant={variant}>search</div>
  ),
}));

vi.mock("@/components/Library/SongList", () => ({
  SongList: ({ variant }: { variant?: string }) => (
    <div data-song-list-variant={variant}>songs</div>
  ),
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

const defaultSidebarProps = {
  onToggleSidebar: vi.fn(),
  sidebarVisible: true,
  integratedWindowHeader: false,
};

describe("Sidebar", () => {
  test("does not render a duplicate import icon beside the local music heading", () => {
    const markup = renderToStaticMarkup(<Sidebar />);

    expect(markup).not.toContain("lucide-cloud-upload");
  });

  test("hides separate-all controls when the library has only media-g songs", () => {
    const markup = renderToStaticMarkup(<Sidebar {...defaultSidebarProps} />);

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

    const markup = renderToStaticMarkup(<Sidebar {...defaultSidebarProps} />);

    expect(markup).not.toContain("sidebar.separateAll");
    expect(markup).not.toContain("sidebar.upgradeAll");
  });

  test("passes the native visual variant through the sidebar composition", () => {
    const markup = renderToStaticMarkup(<Sidebar variant="native" />);

    expect(markup).toContain('data-sidebar-visual-variant="native"');
    expect(markup).toContain('data-search-variant="native"');
    expect(markup).toContain('data-song-list-variant="native"');
  });
});
