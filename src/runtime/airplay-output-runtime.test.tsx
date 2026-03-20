// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { beforeEach, describe, expect, test, vi } from "vitest";
import type { ShortcutPlatform } from "@/lib/app-shortcuts";
import { useAirPlayOutputState } from "./airplay-runtime";

const { mockListen, mockCloseFullscreenPlayer, mockGetShortcutPlatform } =
  vi.hoisted(() => ({
    mockListen: vi.fn(),
    mockCloseFullscreenPlayer: vi.fn(),
    mockGetShortcutPlatform: vi.fn<() => ShortcutPlatform>(() => "mac"),
  }));

vi.mock("@tauri-apps/api/event", () => ({
  listen: mockListen,
}));

vi.mock("@/lib/fullscreen-player", () => ({
  closeFullscreenPlayer: mockCloseFullscreenPlayer,
}));

vi.mock("@/lib/app-shortcuts", async () => {
  const actual = await vi.importActual<typeof import("@/lib/app-shortcuts")>(
    "@/lib/app-shortcuts",
  );

  return {
    ...actual,
    getShortcutPlatform: mockGetShortcutPlatform,
  };
});

function HookHarness() {
  useAirPlayOutputState(true);
  return null;
}

describe("useAirPlayOutputState", () => {
  beforeEach(() => {
    mockListen.mockReset();
    mockCloseFullscreenPlayer.mockReset();
    mockGetShortcutPlatform.mockReset();
    mockGetShortcutPlatform.mockReturnValue("mac");
  });

  test("closes the local fullscreen audience window when AirPlay becomes active", async () => {
    let listener:
      | ((event: {
          payload: {
            active: boolean;
            routeName: string | null;
            mode: "idle" | "lyrics" | "cdg";
          };
        }) => void)
      | null = null;
    mockListen.mockImplementation(async (_eventName, nextListener) => {
      listener = nextListener;
      return vi.fn();
    });

    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<HookHarness />);
    });

    await act(async () => {
      listener?.({
        payload: {
          active: true,
          routeName: "Living Room TV",
          mode: "lyrics",
        },
      });
    });

    expect(mockCloseFullscreenPlayer).toHaveBeenCalledOnce();

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("does not register AirPlay output listeners on non-mac platforms", async () => {
    mockGetShortcutPlatform.mockReturnValue("windows");
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<HookHarness />);
    });

    expect(mockListen).not.toHaveBeenCalled();

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });
});
