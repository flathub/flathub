// @vitest-environment jsdom

import { act } from "react";
import type { ReactNode } from "react";
import { createRoot } from "react-dom/client";
import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { LyricsPanel } from "./LyricsPanel";

const {
  mockListen,
  mockStepPlainTextRemotePage,
  mockPlayerState,
  mockLyricsState,
  mockSettingsState,
  mockSelectSyncDisplayPositionMs,
} = vi.hoisted(() => ({
  mockListen: vi.fn(),
  mockStepPlainTextRemotePage: vi.fn(),
  mockPlayerState: {
    snapshot: {
      song_id: "song-1",
    },
    positionMs: 4000,
    airPlayOutput: {
      active: false,
      routeName: null,
      mode: "idle",
      phase: "idle",
      detail: null,
      displayedPositionMs: null,
      streamGeneration: 0,
      latencyMs: null,
    },
    localAudienceOutputActive: true,
  },
  mockLyricsState: {
    lines: [
      { time_ms: 0, text: "line one", words: null },
      { time_ms: 0, text: "line two", words: null },
      { time_ms: 0, text: "line three", words: null },
      { time_ms: 0, text: "line four", words: null },
    ],
    activeLineIndex: -1,
    offsetMs: 0,
    isLoading: false,
    rawLrc: "line one\nline two\nline three\nline four",
    songId: "song-1",
  },
  mockSettingsState: {
    lyricsFontStep: 0,
  },
  mockSelectSyncDisplayPositionMs: vi.fn(
    (state: { positionMs: number }) => state.positionMs,
  ),
}));

vi.mock("@tauri-apps/api/event", () => ({
  listen: mockListen,
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
    i18n: { changeLanguage: vi.fn() },
  }),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
  selectSyncDisplayPositionMs: mockSelectSyncDisplayPositionMs,
}));

vi.mock("@/stores/lyrics-store", () => ({
  useLyricsStore: (selector: (state: typeof mockLyricsState) => unknown) =>
    selector(mockLyricsState),
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

vi.mock("@/lib/plain-text-page-controls", () => ({
  LOCAL_AUDIENCE_PLAIN_TEXT_PAGE_EVENT:
    "openkara://local-audience-plain-text-page",
  resolvePlainTextRemoteTarget: (
    airPlayOutput: { phase: string },
    localAudienceOutputActive: boolean,
  ) =>
    airPlayOutput.phase !== "idle"
      ? "airplay"
      : localAudienceOutputActive
        ? "local"
        : null,
  stepPlainTextRemotePage: mockStepPlainTextRemotePage,
}));

vi.mock("./LyricLine", () => ({
  LyricLine: ({ line }: { line: { text: string } }) => <div>{line.text}</div>,
}));

vi.mock("./LyricsEditDialog", () => ({
  LyricsEditDialog: () => null,
}));

vi.mock("./LyricsFontSizeControl", () => ({
  LyricsFontSizeControl: () => null,
}));

vi.mock("./LyricsOffsetControl", () => ({
  LyricsOffsetControl: () => null,
}));

vi.mock("./LyricsEmptyState", () => ({
  LyricsEmptyState: () => null,
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({ children }: { children: ReactNode }) => <>{children}</>,
}));

describe("LyricsPanel remote paging", () => {
  beforeEach(() => {
    mockListen.mockReset();
    mockStepPlainTextRemotePage.mockReset();
    mockStepPlainTextRemotePage.mockResolvedValue(true);
    mockPlayerState.airPlayOutput.phase = "idle";
    mockPlayerState.localAudienceOutputActive = true;
    mockLyricsState.lines = [
      { time_ms: 0, text: "line one", words: null },
      { time_ms: 0, text: "line two", words: null },
      { time_ms: 0, text: "line three", words: null },
      { time_ms: 0, text: "line four", words: null },
    ];
    mockLyricsState.songId = "song-1";
    mockLyricsState.rawLrc = "line one\nline two\nline three\nline four";
    mockSelectSyncDisplayPositionMs.mockImplementation(
      (state) => state.positionMs,
    );

    class MockResizeObserver {
      observe() {}
      disconnect() {}
    }

    Object.defineProperty(globalThis, "ResizeObserver", {
      configurable: true,
      value: MockResizeObserver,
    });
    Object.defineProperty(HTMLElement.prototype, "clientHeight", {
      configurable: true,
      get() {
        return this.getAttribute("data-testid") === "lyrics-scroll-viewport"
          ? 230
          : 0;
      },
    });
    Object.defineProperty(HTMLElement.prototype, "getBoundingClientRect", {
      configurable: true,
      value() {
        return {
          width: 600,
          height: this.hasAttribute("data-plain-text-page-measure-line")
            ? 30
            : 20,
          top: 0,
          right: 600,
          bottom: 30,
          left: 0,
          x: 0,
          y: 0,
          toJSON: () => ({}),
        };
      },
    });
  });

  afterEach(() => {
    document.body.innerHTML = "";
  });

  test("clicking the paging buttons only dispatches a remote page step", async () => {
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<LyricsPanel />);
    });

    const viewport = container.querySelector(
      '[data-testid="lyrics-scroll-viewport"]',
    ) as HTMLDivElement;
    viewport.scrollTop = 120;

    const nextButton = container.querySelector(
      '[data-testid="plain-text-page-next"]',
    ) as HTMLButtonElement;

    await act(async () => {
      nextButton.click();
    });

    expect(mockStepPlainTextRemotePage).toHaveBeenCalledWith(
      mockPlayerState.airPlayOutput,
      true,
      "next",
    );
    expect(viewport.scrollTop).toBe(120);

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("audience presentation paginates plain-text lyrics and reacts to page events", async () => {
    let listener:
      | ((event: { payload: { direction: "prev" | "next" } }) => void)
      | null = null;
    mockListen.mockImplementation(async (_eventName, nextListener) => {
      listener = nextListener;
      return vi.fn();
    });

    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(<LyricsPanel presentation="audience" />);
    });

    const viewport = container.querySelector(
      '[data-testid="lyrics-scroll-viewport"]',
    ) as HTMLDivElement;

    expect(viewport.textContent).toContain("line one");
    expect(viewport.textContent).toContain("line two");
    expect(viewport.textContent).not.toContain("line three");

    await act(async () => {
      listener?.({ payload: { direction: "next" } });
    });

    expect(viewport.textContent).not.toContain("line one");
    expect(viewport.textContent).not.toContain("line two");
    expect(viewport.textContent).toContain("line three");
    expect(viewport.textContent).toContain("line four");

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });
});
