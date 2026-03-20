// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { beforeEach, describe, expect, test, vi } from "vitest";
import { MonitorPicker } from "./MonitorPicker";

const {
  mockGetMonitors,
  mockOpenFullscreenPlayer,
  mockSyncAirPlayAudienceState,
} = vi.hoisted(() => ({
  mockGetMonitors: vi.fn(),
  mockOpenFullscreenPlayer: vi.fn(),
  mockSyncAirPlayAudienceState: vi.fn(),
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string, options?: { index?: number }) =>
      (
        ({
          "player.selectMonitor": "Select Monitor",
          "player.localDisplayOutput": "Local Display Output",
          "player.noDisplaysFound": "No displays found yet.",
          "player.monitor": `Monitor ${options?.index ?? ""}`.trim(),
        }) as const
      )[key] ?? key,
  }),
}));

vi.mock("@/lib/fullscreen-player", () => ({
  getMonitors: mockGetMonitors,
  openFullscreenPlayer: mockOpenFullscreenPlayer,
}));

vi.mock("@/lib/tauri", () => ({
  syncAirPlayAudienceState: mockSyncAirPlayAudienceState,
}));

interface MockMonitor {
  name: string | null;
  size: { width: number; height: number };
  position: { x: number; y: number };
}

function buildMonitor(
  name: string,
  index: number,
  overrides: Partial<MockMonitor> = {},
): MockMonitor {
  return {
    name,
    size: { width: 1920, height: 1080 },
    position: { x: 1920 * index, y: 0 },
    ...overrides,
  };
}

async function flushEffects() {
  await act(async () => {
    await Promise.resolve();
  });
}

describe("MonitorPicker", () => {
  beforeEach(() => {
    mockGetMonitors.mockReset();
    mockOpenFullscreenPlayer.mockReset();
    mockSyncAirPlayAudienceState.mockReset();
    mockGetMonitors.mockResolvedValue([buildMonitor("Studio Display", 0)]);
    mockSyncAirPlayAudienceState.mockResolvedValue(undefined);

    (
      globalThis as typeof globalThis & {
        IS_REACT_ACT_ENVIRONMENT?: boolean;
      }
    ).IS_REACT_ACT_ENVIRONMENT = true;

    Object.defineProperty(HTMLElement.prototype, "getBoundingClientRect", {
      configurable: true,
      value: () => ({
        width: 120,
        height: 32,
        top: 24,
        right: 144,
        bottom: 56,
        left: 24,
        x: 24,
        y: 24,
        toJSON: () => ({}),
      }),
    });
  });

  test("renders only the local display section", async () => {
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);
    const anchor = document.createElement("button");
    document.body.appendChild(anchor);

    await act(async () => {
      root.render(
        <MonitorPicker onClose={() => {}} anchorRef={{ current: anchor }} />,
      );
    });
    await flushEffects();

    expect(document.body.textContent).toContain("Local Display Output");
    expect(document.body.textContent).not.toContain("AirPlay Output");
    expect(document.body.textContent).not.toContain(
      "Choose an AirPlay device from the native system control below.",
    );

    await act(async () => {
      root.unmount();
    });
    anchor.remove();
    container.remove();
  });

  test("shows the empty-state copy when no displays are available", async () => {
    mockGetMonitors.mockResolvedValue([]);
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);
    const anchor = document.createElement("button");
    document.body.appendChild(anchor);

    await act(async () => {
      root.render(
        <MonitorPicker onClose={() => {}} anchorRef={{ current: anchor }} />,
      );
    });
    await flushEffects();

    expect(document.body.textContent).toContain("No displays found yet.");

    await act(async () => {
      root.unmount();
    });
    anchor.remove();
    container.remove();
  });

  test("stops AirPlay output before opening a local audience display", async () => {
    const onClose = vi.fn();
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);
    const anchor = document.createElement("button");
    document.body.appendChild(anchor);

    await act(async () => {
      root.render(
        <MonitorPicker onClose={onClose} anchorRef={{ current: anchor }} />,
      );
    });
    await flushEffects();

    const monitorButton = [...document.body.querySelectorAll("button")].find(
      (button) =>
        button.textContent?.includes("Monitor 1") &&
        button.textContent?.includes("1920x1080"),
    );
    expect(monitorButton).toBeTruthy();

    await act(async () => {
      monitorButton?.dispatchEvent(
        new MouseEvent("click", { bubbles: true, cancelable: true }),
      );
    });
    await flushEffects();

    expect(mockSyncAirPlayAudienceState).toHaveBeenCalledWith(
      expect.objectContaining({ mode: "idle" }),
    );
    expect(mockOpenFullscreenPlayer).toHaveBeenCalledWith(0);
    expect(onClose).toHaveBeenCalledOnce();

    await act(async () => {
      root.unmount();
    });
    anchor.remove();
    container.remove();
  });
});
