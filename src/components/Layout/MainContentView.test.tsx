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
  test("marks the main column with the unified shell variant", () => {
    const markup = renderToStaticMarkup(<MainContentView />);

    expect(markup).toContain('data-main-content-visual-variant="unified"');
    expect(markup).toContain('data-playback-stage="true"');
    expect(markup).toContain('data-playback-bar="true"');
  });

  test("overlays settings without unmounting playback content", () => {
    mockSettingsState.isOpen = true;

    const markup = renderToStaticMarkup(<MainContentView />);

    expect(markup).toContain('data-settings-overlay="true"');
    expect(markup).toContain('data-playback-stage="true"');
    expect(markup).not.toContain("data-native-floating-controls");

    mockSettingsState.isOpen = false;
  });
});
