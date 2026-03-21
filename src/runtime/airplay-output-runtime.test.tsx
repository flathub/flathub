// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { beforeEach, describe, expect, test, vi } from "vitest";
import type { ShortcutPlatform } from "@/lib/app-shortcuts";
import { usePlayerStore } from "@/stores/player-store";
import type { AirPlayOutputStateEvent } from "@/types/ipc";
import {
  useAirPlayOutputState,
  useLocalAudienceOutputState,
} from "./airplay-runtime";

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

function LocalAudienceHookHarness() {
  useLocalAudienceOutputState(true);
  return null;
}

describe("useAirPlayOutputState", () => {
  beforeEach(() => {
    mockListen.mockReset();
    mockCloseFullscreenPlayer.mockReset();
    mockGetShortcutPlatform.mockReset();
    mockGetShortcutPlatform.mockReturnValue("mac");
    usePlayerStore.setState({
      snapshot: null,
      positionMs: 0,
      localAudienceOutputActive: false,
      airPlayOutput: {
        active: false,
        audioActive: false,
        routeName: null,
        mode: "idle",
        phase: "idle",
        detail: null,
        displayedPositionMs: null,
        streamGeneration: 0,
        latencyMs: null,
      },
    });
  });

  test("closes the local fullscreen audience window when AirPlay becomes active", async () => {
    let listener:
      | ((event: { payload: AirPlayOutputStateEvent }) => void)
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
          active: false,
          audioActive: false,
          routeName: "Living Room TV",
          mode: "lyrics",
          phase: "buffering",
          detail: "waiting_for_route",
          displayedPositionMs: null,
          streamGeneration: 2,
          latencyMs: null,
        },
      });
    });

    expect(mockCloseFullscreenPlayer).not.toHaveBeenCalled();
    expect(usePlayerStore.getState().airPlayOutput).toMatchObject({
      phase: "buffering",
      displayedPositionMs: null,
      streamGeneration: 2,
    });

    await act(async () => {
      listener?.({
        payload: {
          active: true,
          audioActive: true,
          routeName: "Living Room TV",
          mode: "lyrics",
          phase: "playing",
          detail: null,
          displayedPositionMs: 1250,
          streamGeneration: 3,
          latencyMs: 420,
        },
      });
    });

    expect(mockCloseFullscreenPlayer).toHaveBeenCalledOnce();
    expect(usePlayerStore.getState().airPlayOutput).toMatchObject({
      active: true,
      phase: "playing",
      displayedPositionMs: 1250,
      streamGeneration: 3,
      latencyMs: 420,
    });

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
    expect(usePlayerStore.getState().airPlayOutput.phase).toBe("idle");

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("tracks whether the local audience window is open", async () => {
    let listener: ((event: { payload: { active: boolean } }) => void) | null =
      null;
    mockListen.mockImplementation(async (_eventName, nextListener) => {
      listener = nextListener;
      return vi.fn();
    });

    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<LocalAudienceHookHarness />);
    });

    await act(async () => {
      listener?.({ payload: { active: true } });
    });

    expect(usePlayerStore.getState().localAudienceOutputActive).toBe(true);

    await act(async () => {
      listener?.({ payload: { active: false } });
    });

    expect(usePlayerStore.getState().localAudienceOutputActive).toBe(false);

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("resets local audience state to hidden before listening for window events", async () => {
    usePlayerStore.setState({ localAudienceOutputActive: true });
    mockListen.mockImplementation(async () => vi.fn());

    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<LocalAudienceHookHarness />);
    });

    expect(usePlayerStore.getState().localAudienceOutputActive).toBe(false);

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });
});
