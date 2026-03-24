import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { NativeFloatingControls } from "./NativeFloatingControls";

const { mockLayoutState, mockSettingsState } = vi.hoisted(() => ({
  mockLayoutState: {
    sidebarVisible: true,
    toggleSidebar: vi.fn(),
  },
  mockSettingsState: {
    isOpen: false,
    toggle: vi.fn(),
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/stores/layout-store", () => ({
  useLayoutStore: (selector: (state: typeof mockLayoutState) => unknown) =>
    selector(mockLayoutState),
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({ children }: { children: React.ReactNode }) => children,
}));

vi.mock("@/components/Player/AirPlayRouteButton", () => ({
  AirPlayRouteButton: () => <div data-airplay-route-button="true" />,
}));

vi.mock("@/components/Player/MonitorPicker", () => ({
  MonitorPicker: () => null,
}));

describe("NativeFloatingControls", () => {
  test("renders the right-side floating utility group", () => {
    const markup = renderToStaticMarkup(<NativeFloatingControls />);

    expect(markup).toContain('data-native-floating-controls="true"');
    expect(markup).toContain('data-airplay-route-button="true"');
    expect(markup).toContain('aria-label="player.selectMonitor"');
    expect(markup).toContain('aria-label="toolbar.settings"');
  });

  test("renders a restore toggle when the native sidebar is collapsed", () => {
    mockLayoutState.sidebarVisible = false;

    const markup = renderToStaticMarkup(<NativeFloatingControls />);

    expect(markup).toContain('data-native-sidebar-restore="true"');
    expect(markup).toContain('aria-label="toolbar.toggleSidebar"');

    mockLayoutState.sidebarVisible = true;
  });
});
