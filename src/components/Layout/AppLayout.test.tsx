import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import type { WindowShellState } from "@/lib/window-shell";
import { AppLayout } from "./AppLayout";

const macNativeShellState = {
  chromeVariant: "mac",
  tier: "mac_native",
  toolbarHeight: 56,
  trafficLightInsetLeading: 78,
  sidebarWidth: 260,
} satisfies WindowShellState;

const {
  mockPlayerState,
  mockLayoutState,
  mockQueueState,
  mockLibraryState,
  mockSettingsState,
} = vi.hoisted(() => ({
  mockPlayerState: {
    airPlayOutput: {
      active: true,
      audioActive: true,
      phase: "playing",
    },
  },
  mockLayoutState: {
    sidebarVisible: true,
    toggleSidebar: vi.fn(),
  },
  mockQueueState: {
    isOpen: false,
    toggle: vi.fn(),
  },
  mockLibraryState: {
    importFiles: vi.fn(),
  },
  mockSettingsState: {
    isOpen: false,
    open: vi.fn(),
    toggle: vi.fn(),
  },
}));

vi.mock("@/components/Bootstrap/ModelBootstrapBanner", () => ({
  ModelBootstrapBanner: () => <div data-testid="model-banner" />,
}));

vi.mock("@/components/Layout/GlobalProgressBar", () => ({
  GlobalProgressBar: () => <div data-testid="global-progress" />,
}));

vi.mock("@/components/Library/ImportCdgChoiceDialog", () => ({
  ImportCdgChoiceDialog: () => <div data-testid="import-cdg-dialog" />,
}));

vi.mock("@/components/Player/PlaybackBar", () => ({
  PlaybackBar: () => <div data-testid="playback-bar" />,
}));

vi.mock("@/components/Playback/PlaybackStage", () => ({
  PlaybackStage: ({ presentation = "standard" }: { presentation?: string }) => (
    <div data-testid="playback-stage" data-presentation={presentation} />
  ),
}));

vi.mock("@/components/Player/QueuePanel", () => ({
  QueuePanel: () => <div data-testid="queue-panel" />,
}));

vi.mock("@/components/Settings/SettingsOverlay", () => ({
  SettingsOverlay: () => <div data-testid="settings-overlay" />,
}));

vi.mock("./Sidebar", () => ({
  Sidebar: () => <div data-testid="sidebar" />,
}));

vi.mock("./SidebarRail", () => ({
  SidebarRail: ({ children }: { children: React.ReactNode }) => (
    <div data-testid="sidebar-rail">{children}</div>
  ),
}));

vi.mock("./ToastContainer", () => ({
  ToastContainer: () => <div data-testid="toast-container" />,
}));

vi.mock("./WindowChrome", () => ({
  WindowChrome: ({
    shellState,
  }: {
    shellState: { tier: string; trafficLightInsetLeading: number };
  }) => (
    <div
      data-testid="window-chrome"
      data-shell-tier={shellState.tier}
      data-shell-inset={shellState.trafficLightInsetLeading}
    />
  ),
}));

vi.mock("@/hooks/use-animated-presence", () => ({
  useAnimatedPresence: () => ({
    shouldRender: false,
    className: "",
    onAnimationEnd: vi.fn(),
  }),
}));

vi.mock("@/lib/app-shortcuts", () => ({
  getShortcutPlatform: () => "mac",
  getShortcutDisplay: () => "",
  APP_SHORTCUTS: {
    toggleSettings: { id: "settings.toggle" },
  },
}));

vi.mock("@/lib/window-chrome", () => ({
  getWindowChromeVariant: () => "mac",
}));

vi.mock("@/runtime/menu-runtime", () => ({
  promptImportFiles: vi.fn(),
}));

vi.mock("@/stores/layout-store", () => ({
  useLayoutStore: (selector: (state: typeof mockLayoutState) => unknown) =>
    selector(mockLayoutState),
}));

vi.mock("@/stores/library-store", () => ({
  useLibraryStore: (selector: (state: typeof mockLibraryState) => unknown) =>
    selector(mockLibraryState),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
}));

vi.mock("@/stores/queue-store", () => ({
  useQueueStore: (selector: (state: typeof mockQueueState) => unknown) =>
    selector(mockQueueState),
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

describe("AppLayout", () => {
  test("keeps the main window on the standard playback stage when AirPlay is active", () => {
    const markup = renderToStaticMarkup(
      <AppLayout initialWindowShellState={macNativeShellState} />,
    );

    expect(markup).toContain('data-testid="playback-stage"');
    expect(markup).toContain('data-presentation="standard"');
    expect(markup).not.toContain('data-presentation="audience"');
    expect(markup).toContain('data-window-shell-tier="mac_native"');
    expect(markup).toContain("--window-shell-toolbar-height:56px");
    expect(markup).toContain('data-shell-tier="mac_native"');
  });
});
