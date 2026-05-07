import { beforeEach, describe, expect, test, vi } from "vitest";
import { useLyricsStore } from "./lyrics-store";

const { mockSaveManualLyrics, mockNotifyError, mockRomanizeLyricsLines } =
  vi.hoisted(() => ({
    mockSaveManualLyrics: vi.fn(),
    mockNotifyError: vi.fn(),
    mockRomanizeLyricsLines: vi.fn(),
  }));

vi.mock("@/lib/tauri", () => ({
  saveManualLyrics: mockSaveManualLyrics,
}));

vi.mock("@/lib/errors", () => ({
  notifyError: mockNotifyError,
}));

vi.mock("@/lib/lyrics-romanizer", () => ({
  romanizeLyricsLines: mockRomanizeLyricsLines,
}));

describe("lyrics-store saveManualLyrics", () => {
  beforeEach(() => {
    mockSaveManualLyrics.mockReset();
    mockNotifyError.mockReset();
    mockRomanizeLyricsLines.mockReset();
    useLyricsStore.setState({
      songId: "song-1",
      lines: [],
      source: null,
      offsetMs: 0,
      rawLrc: "[00:00.00]Original",
      activeLineIndex: -1,
      isLoading: false,
      romanizedLines: [],
      isRomanizing: false,
      showRomanized: false,
    });
  });

  test("returns true and updates lyrics on save success", async () => {
    mockSaveManualLyrics.mockResolvedValue({
      song_id: "song-1",
      lines: [{ time_ms: 0, text: "Updated" }],
      source: "manual",
      offset_ms: 120,
      raw_lrc: "[00:00.00]Updated",
    });

    const result = await useLyricsStore
      .getState()
      .saveManualLyrics("song-1", "[00:00.00]Updated");

    expect(result).toBe(true);
    expect(useLyricsStore.getState()).toMatchObject({
      songId: "song-1",
      source: "manual",
      offsetMs: 120,
      rawLrc: "[00:00.00]Updated",
    });
    expect(mockNotifyError).not.toHaveBeenCalled();
  });

  test("returns false and keeps current lyrics when save fails", async () => {
    const error = new Error("Lyrics save failed");
    mockSaveManualLyrics.mockRejectedValue(error);

    const result = await useLyricsStore
      .getState()
      .saveManualLyrics("song-1", "[00:00.00]Updated");

    expect(result).toBe(false);
    expect(useLyricsStore.getState()).toMatchObject({
      songId: "song-1",
      rawLrc: "[00:00.00]Original",
    });
    expect(mockNotifyError).toHaveBeenCalledWith(error);
  });

  test("romanizes current lyric lines on first pronunciation toggle", async () => {
    mockRomanizeLyricsLines.mockResolvedValue(["ni hao", "shi jie"]);
    useLyricsStore.setState({
      lines: [
        { time_ms: 0, text: "你好", words: [] },
        { time_ms: 1000, text: "世界", words: [] },
      ],
    });

    useLyricsStore.getState().toggleRomanized();
    await vi.waitFor(() =>
      expect(useLyricsStore.getState().romanizedLines).toEqual([
        "ni hao",
        "shi jie",
      ]),
    );

    expect(mockRomanizeLyricsLines).toHaveBeenCalledWith(
      ["你好", "世界"],
      null,
    );
    expect(useLyricsStore.getState().showRomanized).toBe(true);
    expect(useLyricsStore.getState().isRomanizing).toBe(false);
  });
});
