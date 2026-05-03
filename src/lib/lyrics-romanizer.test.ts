import { beforeEach, describe, expect, test, vi } from "vitest";

const { mockCreateRomanizer, mockRomanizeLines } = vi.hoisted(() => ({
  mockCreateRomanizer: vi.fn(),
  mockRomanizeLines: vi.fn(),
}));

vi.mock("lyric-romanizer", () => ({
  createRomanizer: mockCreateRomanizer,
}));

describe("romanizeLyricsLines", () => {
  beforeEach(() => {
    vi.resetModules();
    mockCreateRomanizer.mockReset();
    mockRomanizeLines.mockReset();
    mockCreateRomanizer.mockReturnValue({
      romanizeLines: mockRomanizeLines,
    });
    mockRomanizeLines.mockResolvedValue({
      script: "chinese",
      lines: ["ni hao"],
    });
  });

  test("keeps Latin lyrics on the detector path without loading the full romanizer", async () => {
    const { romanizeLyricsLines } = await import("./lyrics-romanizer");

    await expect(romanizeLyricsLines(["Hello world"])).resolves.toEqual([
      "Hello world",
    ]);

    expect(mockCreateRomanizer).not.toHaveBeenCalled();
  });

  test("loads and reuses the full romanizer for non-Latin lyrics", async () => {
    const { romanizeLyricsLines } = await import("./lyrics-romanizer");

    await expect(romanizeLyricsLines(["你好"])).resolves.toEqual(["ni hao"]);
    await romanizeLyricsLines(["世界"]);

    expect(mockCreateRomanizer).toHaveBeenCalledTimes(1);
    expect(mockRomanizeLines).toHaveBeenCalledTimes(2);
  });
});
