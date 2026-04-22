import { describe, expect, test } from "vitest";

describe("use-playback-runtime wiring", () => {
  test("registers upload progress listeners alongside separation listeners", async () => {
    const { default: src } = await import("./use-playback-runtime.ts?raw");

    expect(src).toContain("upload-progress");
    expect(src).toContain("upload-complete");
    expect(src).toContain("upload-error");
    expect(src).toContain("updateUploadStatus");
    expect(src).toContain("clearUploadStatus");
    expect(src).toContain("separation-progress");
  });
});
