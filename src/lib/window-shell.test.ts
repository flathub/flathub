import { describe, expect, test } from "vitest";
import {
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
        sidebarWidth: 380,
      }),
    ).toMatchObject({
      chromeVariant: "mac",
      tier: "mac_native",
      toolbarHeight: 56,
      trafficLightInsetLeading: 78,
      sidebarWidth: 380,
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
