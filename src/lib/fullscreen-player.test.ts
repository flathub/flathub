import { describe, expect, test, vi } from "vitest";
import { closeFullscreenPlayer } from "./fullscreen-player";

const {
  mockCloseCurrent,
  mockCloseByLabel,
  mockGetByLabel,
  mockGetCurrentWebviewWindow,
} = vi.hoisted(() => ({
  mockCloseCurrent: vi.fn(),
  mockCloseByLabel: vi.fn(),
  mockGetByLabel: vi.fn(),
  mockGetCurrentWebviewWindow: vi.fn(() => ({
    label: "fullscreen-player",
    close: mockCloseCurrent,
  })),
}));

vi.mock("@tauri-apps/api/webviewWindow", () => ({
  WebviewWindow: {
    getByLabel: mockGetByLabel,
  },
  getCurrentWebviewWindow: mockGetCurrentWebviewWindow,
}));

vi.mock("@tauri-apps/api/window", () => ({
  availableMonitors: vi.fn(),
}));

describe("closeFullscreenPlayer", () => {
  test("closes the current fullscreen window directly", async () => {
    mockGetByLabel.mockResolvedValue({ close: mockCloseByLabel });

    await closeFullscreenPlayer();

    expect(mockCloseCurrent).toHaveBeenCalledOnce();
    expect(mockCloseByLabel).not.toHaveBeenCalled();
  });
});
