import { vi } from "vitest";

vi.mock("lyric-romanizer", () => ({
  createRomanizer: () => ({
    romanizeLines: async (lines: string[]) => ({
      script: "latin",
      lines,
    }),
  }),
  isLatinScript: () => true,
  detectScript: () => "latin" as const,
}));
