import { beforeEach, describe, expect, test, vi } from "vitest";
import { useLibraryStore } from "./library-store";

const { mockUpdateSongMetadata, mockNotifyError } = vi.hoisted(() => ({
  mockUpdateSongMetadata: vi.fn(),
  mockNotifyError: vi.fn(),
}));

vi.mock("@/lib/tauri", () => ({
  updateSongMetadata: mockUpdateSongMetadata,
}));

vi.mock("@/lib/errors", () => ({
  notifyError: mockNotifyError,
}));

describe("library-store updateSongMetadata", () => {
  beforeEach(() => {
    mockUpdateSongMetadata.mockReset();
    mockNotifyError.mockReset();
    useLibraryStore.setState({
      songs: [
        {
          hash: "song-1",
          title: "Original Title",
          artist: "Original Artist",
          album: null,
          file_path: "/music/original.mp3",
          cdg_path: null,
          media_g_container: null,
          duration_ms: 123000,
          cover_art: null,
          imported_at: 0,
          original_ext: null,
        },
      ],
      searchQuery: "",
      isImporting: false,
      importErrors: [],
      selectedSongIds: new Set<string>(),
      lastClickedSongId: null,
      separationStatuses: {},
      filter: "all",
      batchSeparation: null,
    });
  });

  test("returns true and updates the song on save success", async () => {
    mockUpdateSongMetadata.mockResolvedValue({
      title: "Updated Title",
      artist: "Updated Artist",
    });

    const result = await useLibraryStore
      .getState()
      .updateSongMetadata("song-1", "Updated Title", "Updated Artist");

    expect(result).toBe(true);
    expect(useLibraryStore.getState().songs[0]).toMatchObject({
      title: "Updated Title",
      artist: "Updated Artist",
    });
    expect(mockNotifyError).not.toHaveBeenCalled();
  });

  test("returns false and keeps the current song when save fails", async () => {
    const error = new Error("Save failed");
    mockUpdateSongMetadata.mockRejectedValue(error);

    const result = await useLibraryStore
      .getState()
      .updateSongMetadata("song-1", "Updated Title", "Updated Artist");

    expect(result).toBe(false);
    expect(useLibraryStore.getState().songs[0]).toMatchObject({
      title: "Original Title",
      artist: "Original Artist",
    });
    expect(mockNotifyError).toHaveBeenCalledWith(error);
  });
});
