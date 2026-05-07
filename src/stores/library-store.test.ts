import { beforeEach, describe, expect, test, vi } from "vitest";
import { useLibraryStore } from "./library-store";

const {
  mockUpdateSongMetadata,
  mockExtractEmbeddedCoverArt,
  mockImportSongs,
  mockGetLibrary,
  mockSetSongsInstrumental,
  mockInvalidateCoverArtUrl,
  mockNotifyError,
} = vi.hoisted(() => ({
  mockUpdateSongMetadata: vi.fn(),
  mockExtractEmbeddedCoverArt: vi.fn(),
  mockImportSongs: vi.fn(),
  mockGetLibrary: vi.fn(),
  mockSetSongsInstrumental: vi.fn(),
  mockInvalidateCoverArtUrl: vi.fn(),
  mockNotifyError: vi.fn(),
}));

vi.mock("@/lib/tauri", () => ({
  importSongs: mockImportSongs,
  getLibrary: mockGetLibrary,
  updateSongMetadata: mockUpdateSongMetadata,
  setSongsInstrumental: mockSetSongsInstrumental,
  extractEmbeddedCoverArt: mockExtractEmbeddedCoverArt,
}));

vi.mock("@/lib/cover-art", () => ({
  invalidateCoverArtUrl: mockInvalidateCoverArtUrl,
}));

vi.mock("@/lib/errors", () => ({
  notifyError: mockNotifyError,
}));

describe("library-store updateSongMetadata", () => {
  beforeEach(() => {
    mockUpdateSongMetadata.mockReset();
    mockExtractEmbeddedCoverArt.mockReset();
    mockImportSongs.mockReset();
    mockGetLibrary.mockReset();
    mockSetSongsInstrumental.mockReset();
    mockInvalidateCoverArtUrl.mockReset();
    mockNotifyError.mockReset();
    useLibraryStore.setState({
      songs: [
        {
          hash: "song-1",
          title: "Original Title",
          artist: "Original Artist",
          album: null,
          file_path: "/music/original.mp3",
          audio_source_kind: "original",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          language: null,
          duration_ms: 123000,
          cover_art: null,
          imported_at: 0,
          original_ext: null,
        },
        {
          hash: "song-2",
          title: "Second Song",
          artist: "Second Artist",
          album: null,
          file_path: "/music/second.mp3",
          audio_source_kind: "original",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          language: null,
          duration_ms: 456000,
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
      uploadStatuses: {},
      filter: "all",
      batchSeparation: null,
      pendingImportCdgChoice: null,
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

  test("returns true and updates selected songs when instrumental state changes", async () => {
    mockSetSongsInstrumental.mockResolvedValue([
      {
        hash: "song-1",
        title: "Original Title",
        artist: "Original Artist",
        album: null,
        file_path: "/music/original.mp3",
        audio_source_kind: "original",
        cdg_path: null,
        media_g_container: null,
        instrumental: true,
        language: null,
        duration_ms: 123000,
        cover_art: null,
        imported_at: 0,
        original_ext: null,
      },
    ]);

    const result = await useLibraryStore
      .getState()
      .setSongsInstrumental(["song-1"], true);

    expect(result).toBe(true);
    expect(mockSetSongsInstrumental).toHaveBeenCalledWith(["song-1"], true);
    expect(useLibraryStore.getState().songs[0].instrumental).toBe(true);
    expect(mockNotifyError).not.toHaveBeenCalled();
  });

  test("prompts for ambiguous CDG selections before importing songs", async () => {
    const promptForCdgChoice = vi.fn().mockResolvedValue("/tmp/track.flac");

    mockImportSongs.mockResolvedValue({ imported: [], failed: [] });
    mockGetLibrary.mockResolvedValue([]);

    useLibraryStore.setState({ promptForCdgChoice });

    await useLibraryStore
      .getState()
      .importFiles(["/tmp/track.mp3", "/tmp/track.flac", "/tmp/track.cdg"]);

    expect(promptForCdgChoice).toHaveBeenCalledWith({
      cdgPath: "/tmp/track.cdg",
      audioCandidates: ["/tmp/track.flac", "/tmp/track.mp3"],
      stem: "track",
    });
    expect(mockImportSongs).toHaveBeenCalledWith(
      ["/tmp/track.flac", "/tmp/track.cdg"],
      {
        explicit_cdg_by_audio_path: {
          "/tmp/track.flac": "/tmp/track.cdg",
        },
      },
    );
  });

  test("applies only successful cover-art refreshes and reports individual failures", async () => {
    const failure = new Error("missing artwork");
    mockExtractEmbeddedCoverArt.mockResolvedValue({
      updated_songs: [
        {
          hash: "song-1",
          title: "Original Title",
          artist: "Original Artist",
          album: null,
          file_path: "/music/original.mp3",
          audio_source_kind: "original",
          cdg_path: null,
          media_g_container: null,
          instrumental: false,
          language: null,
          duration_ms: 123000,
          cover_art: [0xff, 0xd8, 0x00],
          imported_at: 0,
          original_ext: null,
        },
      ],
      failed: [
        {
          song_id: "song-2",
          error: failure,
        },
      ],
    });

    const result = await useLibraryStore
      .getState()
      .extractEmbeddedCoverArt(["song-1", "song-2"]);

    expect(result).toBe(true);
    expect(mockExtractEmbeddedCoverArt).toHaveBeenCalledWith([
      "song-1",
      "song-2",
    ]);
    expect(mockInvalidateCoverArtUrl).toHaveBeenCalledWith("song-1");
    expect(useLibraryStore.getState().songs[0].cover_art).toEqual([
      0xff, 0xd8, 0x00,
    ]);
    expect(useLibraryStore.getState().songs[1].cover_art).toBeNull();
    expect(mockNotifyError).toHaveBeenCalledWith(failure);
  });

  test("returns false when every cover-art extraction fails", async () => {
    const error = new Error("all failed");
    mockExtractEmbeddedCoverArt.mockResolvedValue({
      updated_songs: [],
      failed: [
        {
          song_id: "song-1",
          error,
        },
      ],
    });

    const result = await useLibraryStore
      .getState()
      .extractEmbeddedCoverArt(["song-1"]);

    expect(result).toBe(false);
    expect(useLibraryStore.getState().songs[0].cover_art).toBeNull();
    expect(mockInvalidateCoverArtUrl).not.toHaveBeenCalled();
    expect(mockNotifyError).toHaveBeenCalledWith(error);
  });

  test("tracks upload progress and clears individual upload statuses", () => {
    useLibraryStore.getState().updateUploadStatus({
      song_id: "song-1",
      state: "running",
      percent: 35,
      remote_library_id: null,
      detail: null,
      error: null,
    });

    expect(useLibraryStore.getState().uploadStatuses["song-1"]).toMatchObject({
      state: "running",
      percent: 35,
    });

    useLibraryStore.getState().clearUploadStatus("song-1");

    expect(useLibraryStore.getState().uploadStatuses["song-1"]).toBeUndefined();
  });
});
