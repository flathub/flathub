import { beforeEach, describe, expect, test, vi } from "vitest";

const { mockInvoke } = vi.hoisted(() => ({
  mockInvoke: vi.fn(),
}));

vi.mock("@tauri-apps/api/core", () => ({
  invoke: mockInvoke,
}));

import { getCdgFrame } from "./tauri";

describe("getCdgFrame", () => {
  beforeEach(() => {
    mockInvoke.mockReset();
    mockInvoke.mockResolvedValue(null);
  });

  test("sends the backend positionMs payload name", async () => {
    await getCdgFrame(123.6);

    expect(mockInvoke).toHaveBeenCalledWith("get_cdg_frame", {
      positionMs: 124,
    });
  });
});
