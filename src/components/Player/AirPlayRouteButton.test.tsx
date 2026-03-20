// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { beforeEach, describe, expect, test, vi } from "vitest";
import type { ShortcutPlatform } from "@/lib/app-shortcuts";
import { AirPlayRouteButton } from "./AirPlayRouteButton";

const { mockSyncAirPlayRoutePicker, mockGetShortcutPlatform, mockListen } =
  vi.hoisted(() => ({
    mockSyncAirPlayRoutePicker: vi.fn(),
    mockGetShortcutPlatform: vi.fn<() => ShortcutPlatform>(() => "mac"),
    mockListen: vi.fn(),
  }));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) =>
      (
        ({
          "player.airPlayOutput": "AirPlay Output",
        }) as const
      )[key] ?? key,
  }),
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({
    children,
    label,
  }: {
    children: React.ReactNode;
    label: string;
  }) => <span data-tooltip-label={label}>{children}</span>,
}));

vi.mock("@/lib/app-shortcuts", () => ({
  getShortcutPlatform: mockGetShortcutPlatform,
}));

vi.mock("@/lib/tauri", () => ({
  syncAirPlayRoutePicker: mockSyncAirPlayRoutePicker,
}));

vi.mock("@tauri-apps/api/event", () => ({
  listen: mockListen,
}));

async function flushEffects() {
  await act(async () => {
    await Promise.resolve();
  });
}

describe("AirPlayRouteButton", () => {
  beforeEach(() => {
    mockSyncAirPlayRoutePicker.mockReset();
    mockGetShortcutPlatform.mockReset();
    mockGetShortcutPlatform.mockReturnValue("mac");
    mockSyncAirPlayRoutePicker.mockResolvedValue(undefined);
    mockListen.mockReset();
    mockListen.mockResolvedValue(() => {});

    (
      globalThis as typeof globalThis & {
        IS_REACT_ACT_ENVIRONMENT?: boolean;
      }
    ).IS_REACT_ACT_ENVIRONMENT = true;

    Object.defineProperty(HTMLElement.prototype, "getBoundingClientRect", {
      configurable: true,
      value: () => ({
        width: 36,
        height: 36,
        top: 20,
        right: 200,
        bottom: 56,
        left: 164,
        x: 164,
        y: 20,
        toJSON: () => ({}),
      }),
    });
  });

  test("mounts the native route picker inside the toolbar on macOS", async () => {
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<AirPlayRouteButton />);
    });
    await flushEffects();

    expect(mockSyncAirPlayRoutePicker).toHaveBeenCalledWith(
      expect.objectContaining({
        left: expect.any(Number),
        top: expect.any(Number),
        width: expect.any(Number),
        height: expect.any(Number),
      }),
    );

    await act(async () => {
      root.unmount();
    });

    expect(mockSyncAirPlayRoutePicker).toHaveBeenLastCalledWith(null);
    container.remove();
  });

  test("does not mount the native route picker on non-mac platforms", async () => {
    mockGetShortcutPlatform.mockReturnValue("windows");
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<AirPlayRouteButton />);
    });
    await flushEffects();

    expect(container.textContent).toBe("");
    expect(mockSyncAirPlayRoutePicker).not.toHaveBeenCalled();

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });
});
