import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { MainContentView } from "./MainContentView";

const { mockSettingsState, mockQueueState } = vi.hoisted(() => ({
  mockSettingsState: {
    isOpen: false,
  },
  mockQueueState: {
    isOpen: false,
  },
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

vi.mock("@/stores/queue-store", () => ({
  useQueueStore: (selector: (state: typeof mockQueueState) => unknown) =>
    selector(mockQueueState),
}));

vi.mock("@/hooks/use-animated-presence", () => ({
  useAnimatedPresence: () => ({
    shouldRender: false,
    className: "",
    onAnimationEnd: undefined,
  }),
}));

vi.mock("@/components/Layout/NativeFloatingControls", () => ({
  NativeFloatingControls: () => <div data-native-floating-controls="true" />,
}));

vi.mock("@/components/Layout/GlobalProgressBar", () => ({
  GlobalProgressBar: () => null,
}));

vi.mock("@/components/Playback/PlaybackStage", () => ({
  PlaybackStage: () => <div data-playback-stage="true" />,
}));

vi.mock("@/components/Player/PlaybackBar", () => ({
  PlaybackBar: () => <div data-playback-bar="true" />,
}));

vi.mock("@/components/Settings/SettingsOverlay", () => ({
  SettingsOverlay: () => <div data-settings-overlay="true" />,
}));

vi.mock("@/components/Bootstrap/ModelBootstrapBanner", () => ({
  ModelBootstrapBanner: () => null,
}));

vi.mock("@/components/Player/QueuePanel", () => ({
  QueuePanel: () => null,
}));

describe("MainContentView", () => {
  test("keeps native floating controls visible even while settings are open", () => {
    mockSettingsState.isOpen = true;

    const markup = renderToStaticMarkup(
      <MainContentView shellTier="mac_native" />,
    );

    expect(markup).toContain('data-native-floating-controls="true"');
    expect(markup).toContain('data-settings-overlay="true"');

    mockSettingsState.isOpen = false;
  });
});
