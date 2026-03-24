// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { beforeEach, describe, expect, test, vi } from "vitest";
import { getNativeWindowShellState, useWindowShellState } from "./window-shell";

vi.mock("@/lib/app-shortcuts", () => ({
  getShortcutPlatform: () => "mac",
}));

const { mockGetWindowShellState } = vi.hoisted(() => ({
  mockGetWindowShellState: vi.fn(),
}));

vi.mock("@/lib/tauri", () => ({
  getWindowShellState: mockGetWindowShellState,
}));

describe("useWindowShellState", () => {
  beforeEach(() => {
    (
      globalThis as typeof globalThis & { IS_REACT_ACT_ENVIRONMENT?: boolean }
    ).IS_REACT_ACT_ENVIRONMENT = true;
  });

  test("hydrates real AppKit metrics even when native child webviews start from fallback tokens", async () => {
    mockGetWindowShellState.mockResolvedValue({
      chrome_variant: "mac",
      tier: "mac_native",
      toolbar_height: 56,
      traffic_light_inset_leading: 92,
      sidebar_width: 436,
      sidebar_webview_label: "main-sidebar",
      main_content_webview_label: "main",
    });

    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    function Probe() {
      const state = useWindowShellState(getNativeWindowShellState(), "mac");

      return (
        <div
          data-leading={state.trafficLightInsetLeading}
          data-width={state.sidebarWidth}
        />
      );
    }

    await act(async () => {
      root.render(<Probe />);
    });

    expect(container.innerHTML).toContain('data-leading="92"');
    expect(container.innerHTML).toContain('data-width="436"');

    await act(async () => {
      root.unmount();
    });
  });
});
