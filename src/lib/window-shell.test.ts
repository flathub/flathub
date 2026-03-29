import { describe, expect, test } from "vitest";
import {
  createWindowShellStyle,
  getDefaultWindowShellState,
  resolveWindowShellState,
} from "./window-shell";

describe("window shell helpers", () => {
  test("defaults macOS to the intentional legacy shell before native state arrives", () => {
    expect(getDefaultWindowShellState("mac")).toMatchObject({
      chromeVariant: "mac",
      tier: "mac_legacy",
      toolbarHeight: 52,
      trafficLightInsetLeading: 64,
    });
  });

  test("preserves the native mac shell profile returned by the backend", () => {
    expect(
      resolveWindowShellState("mac", {
        chromeVariant: "mac",
        tier: "mac_native",
        toolbarHeight: 56,
        trafficLightInsetLeading: 78,
        sidebarHeaderHeight: 40,
        sidebarWidth: 380,
      } as never),
    ).toMatchObject({
      chromeVariant: "mac",
      tier: "mac_native",
      toolbarHeight: 56,
      trafficLightInsetLeading: 78,
      sidebarHeaderHeight: 40,
      sidebarWidth: 380,
    });
  });

  test("exposes the native sidebar header height token for traffic-light-safe layout", () => {
    const style = createWindowShellStyle(
      resolveWindowShellState("mac", {
        chromeVariant: "mac",
        tier: "mac_native",
        toolbarHeight: 56,
        trafficLightInsetLeading: 90,
        sidebarHeaderHeight: 40,
        sidebarWidth: 420,
      } as never),
    );

    expect(style).toMatchObject({
      "--window-shell-leading-controls-space": "90px",
      "--window-shell-sidebar-header-height": "40px",
      "--window-shell-sidebar-width": "420px",
    });
  });

  test("forces non-mac platforms back to the desktop shell even if a mac profile leaks through", () => {
    expect(
      resolveWindowShellState("windows", {
        chromeVariant: "mac",
        tier: "mac_native",
        toolbarHeight: 56,
        trafficLightInsetLeading: 78,
      }),
    ).toMatchObject({
      chromeVariant: "desktop",
      tier: "desktop",
      toolbarHeight: 48,
      trafficLightInsetLeading: 0,
    });
  });
});
