import { afterEach, describe, expect, test, vi } from "vitest";

import { fetchReleaseByTag } from "../scripts/release-metadata.mjs";

describe("release metadata helpers", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  test("falls back to the releases list when a draft release is hidden from the tag endpoint", async () => {
    const draftRelease = {
      tag_name: "v0.8.1",
      draft: true,
      assets: [{ name: "OpenKara_0.8.1_x64-setup.exe" }],
    };
    const fetchMock = vi
      .fn<typeof fetch>()
      .mockResolvedValueOnce(
        new Response("", { status: 404, statusText: "Not Found" }),
      )
      .mockResolvedValueOnce(
        Response.json([{ tag_name: "v0.8.0" }, draftRelease]),
      );

    vi.stubGlobal("fetch", fetchMock);

    await expect(
      fetchReleaseByTag({
        owner: "thedavidweng",
        repo: "OpenKara",
        tag: "v0.8.1",
      }),
    ).resolves.toEqual(draftRelease);

    expect(fetchMock).toHaveBeenNthCalledWith(
      2,
      "https://api.github.com/repos/thedavidweng/OpenKara/releases?per_page=100",
      expect.objectContaining({
        headers: expect.objectContaining({
          Accept: "application/vnd.github+json",
        }),
      }),
    );
  });
});
