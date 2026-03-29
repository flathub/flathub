import { useEffect, useState, type CSSProperties } from "react";
import {
  getShortcutPlatform,
  type ShortcutPlatform,
} from "@/lib/app-shortcuts";
import { getWindowShellState as getWindowShellStateSnapshot } from "@/lib/tauri";
import type {
  WindowShellChromeVariant,
  WindowShellStateSnapshot,
  WindowShellTier,
} from "@/types/ipc";

export interface WindowShellState {
  chromeVariant: WindowShellChromeVariant;
  tier: WindowShellTier;
  toolbarHeight: number;
  trafficLightInsetLeading: number;
  sidebarHeaderHeight: number;
  sidebarWidth: number;
}

const DESKTOP_WINDOW_SHELL_STATE: WindowShellState = {
  chromeVariant: "desktop",
  tier: "desktop",
  toolbarHeight: 48,
  trafficLightInsetLeading: 0,
  sidebarHeaderHeight: 0,
  sidebarWidth: 260,
};

const MAC_LEGACY_WINDOW_SHELL_STATE: WindowShellState = {
  chromeVariant: "mac",
  tier: "mac_legacy",
  toolbarHeight: 52,
  trafficLightInsetLeading: 64,
  sidebarHeaderHeight: 0,
  sidebarWidth: 260,
};

const MAC_NATIVE_WINDOW_SHELL_STATE: WindowShellState = {
  chromeVariant: "mac",
  tier: "mac_native",
  toolbarHeight: 56,
  trafficLightInsetLeading: 78,
  sidebarHeaderHeight: 40,
  sidebarWidth: 420,
};

export function getNativeWindowShellState(): WindowShellState {
  return { ...MAC_NATIVE_WINDOW_SHELL_STATE };
}

function isPositiveNumber(value: unknown): value is number {
  return typeof value === "number" && Number.isFinite(value) && value > 0;
}

function snapshotToWindowShellState(
  snapshot: WindowShellStateSnapshot,
): WindowShellState {
  return {
    chromeVariant: snapshot.chrome_variant,
    tier: snapshot.tier,
    toolbarHeight: snapshot.toolbar_height,
    trafficLightInsetLeading: snapshot.traffic_light_inset_leading,
    sidebarHeaderHeight: snapshot.sidebar_header_height,
    sidebarWidth: snapshot.sidebar_width,
  };
}

export function getDefaultWindowShellState(
  platform: ShortcutPlatform,
): WindowShellState {
  return platform === "mac"
    ? { ...MAC_LEGACY_WINDOW_SHELL_STATE }
    : { ...DESKTOP_WINDOW_SHELL_STATE };
}

export function resolveWindowShellState(
  platform: ShortcutPlatform,
  state?: Partial<WindowShellState> | null,
): WindowShellState {
  if (platform !== "mac") {
    return { ...DESKTOP_WINDOW_SHELL_STATE };
  }

  const fallback =
    state?.tier === "mac_native"
      ? MAC_NATIVE_WINDOW_SHELL_STATE
      : MAC_LEGACY_WINDOW_SHELL_STATE;

  return {
    chromeVariant:
      state?.chromeVariant === "mac" ? "mac" : fallback.chromeVariant,
    tier: state?.tier === "mac_native" ? "mac_native" : fallback.tier,
    toolbarHeight: isPositiveNumber(state?.toolbarHeight)
      ? state.toolbarHeight
      : fallback.toolbarHeight,
    trafficLightInsetLeading: isPositiveNumber(state?.trafficLightInsetLeading)
      ? state.trafficLightInsetLeading
      : fallback.trafficLightInsetLeading,
    sidebarHeaderHeight: isPositiveNumber(state?.sidebarHeaderHeight)
      ? state.sidebarHeaderHeight
      : fallback.sidebarHeaderHeight,
    sidebarWidth: isPositiveNumber(state?.sidebarWidth)
      ? state.sidebarWidth
      : fallback.sidebarWidth,
  };
}

export function createWindowShellStyle(state: WindowShellState): CSSProperties {
  return {
    "--window-shell-leading-controls-space": `${state.trafficLightInsetLeading}px`,
    "--window-shell-sidebar-header-height": `${state.sidebarHeaderHeight}px`,
    "--window-shell-sidebar-width": `${state.sidebarWidth}px`,
    "--window-shell-toolbar-height": `${state.toolbarHeight}px`,
  } as CSSProperties;
}

export function useWindowShellState(
  initialState?: WindowShellState,
  platform: ShortcutPlatform = getShortcutPlatform(),
): WindowShellState {
  const resolvedInitialState = resolveWindowShellState(platform, initialState);
  const [nativeState, setNativeState] = useState<WindowShellState | null>(null);
  const shouldHydrateNativeSnapshot =
    platform === "mac" && (!initialState || initialState.tier === "mac_native");

  useEffect(() => {
    if (!shouldHydrateNativeSnapshot) {
      return;
    }

    let cancelled = false;

    getWindowShellStateSnapshot()
      .then((snapshot) => {
        if (cancelled) {
          return;
        }

        setNativeState(
          resolveWindowShellState(
            platform,
            snapshotToWindowShellState(snapshot),
          ),
        );
      })
      .catch(() => {
        if (!cancelled) {
          setNativeState(getDefaultWindowShellState(platform));
        }
      });

    return () => {
      cancelled = true;
    };
  }, [platform, shouldHydrateNativeSnapshot]);

  if (!shouldHydrateNativeSnapshot) {
    return resolvedInitialState;
  }

  return nativeState ?? resolvedInitialState;
}
