import { renderToStaticMarkup } from "react-dom/server";
import { beforeEach, describe, expect, test, vi } from "vitest";
import { LyricsPanel } from "./LyricsPanel";

const { mockPlayerState, mockLyricsState, mockSettingsState } = vi.hoisted(
  () => ({
    mockPlayerState: {
      snapshot: {
        song_id: "song-1",
      },
      positionMs: 4000,
    },
    mockLyricsState: {
      lines: [
        {
          time_ms: 0,
          text: "line one",
          words: null,
        },
      ],
      activeLineIndex: 0,
      offsetMs: 0,
      isLoading: false,
      rawLrc: "[00:00.00]line one",
      songId: "song-1",
      adjustOffset: vi.fn(),
    },
    mockSettingsState: {
      lyricsFontStep: 0,
      adjustLyricsFontStep: vi.fn(),
      resetLyricsFontStep: vi.fn(),
    },
  }),
);

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
    i18n: { changeLanguage: vi.fn() },
  }),
}));

vi.mock("@/stores/player-store", () => ({
  usePlayerStore: (selector: (state: typeof mockPlayerState) => unknown) =>
    selector(mockPlayerState),
}));

vi.mock("@/stores/lyrics-store", () => ({
  useLyricsStore: (selector: (state: typeof mockLyricsState) => unknown) =>
    selector(mockLyricsState),
}));

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: (selector: (state: typeof mockSettingsState) => unknown) =>
    selector(mockSettingsState),
}));

describe("LyricsPanel contextual reveal", () => {
  beforeEach(() => {
    mockPlayerState.snapshot = {
      song_id: "song-1",
    };
    mockPlayerState.positionMs = 4000;

    mockLyricsState.lines = [
      {
        time_ms: 0,
        text: "line one",
        words: null,
      },
    ];
    mockLyricsState.activeLineIndex = 0;
    mockLyricsState.offsetMs = 0;
    mockLyricsState.isLoading = false;
    mockLyricsState.rawLrc = "[00:00.00]line one";
    mockLyricsState.songId = "song-1";
    mockLyricsState.adjustOffset.mockReset();
    mockSettingsState.lyricsFontStep = 0;
  });

  test("renders utility chrome in an overlay layer without layout controls at rest", () => {
    const markup = renderToStaticMarkup(<LyricsPanel />);

    expect(markup).toContain("contextual-reveal absolute right-4 top-4");
    expect(markup).toContain("absolute inset-x-0 bottom-0");
    expect(markup).not.toContain('data-visible="true"');
  });

  test("keeps lyric utility chrome visible when offset is non-zero", () => {
    mockLyricsState.offsetMs = 500;

    const markup = renderToStaticMarkup(<LyricsPanel />);

    expect(markup).toContain('data-visible="true"');
    expect(markup).toContain("+0.5s");
  });

  test("keeps lyric utility chrome visible when font size is non-default", () => {
    mockSettingsState.lyricsFontStep = 1;

    const markup = renderToStaticMarkup(<LyricsPanel />);

    expect(markup).toContain('data-visible="true"');
    expect(markup).toContain("lyrics.fontSizeResetShort");
  });

  test("renders plain-text lyrics at full brightness when no timestamps exist", () => {
    mockLyricsState.lines = [
      {
        time_ms: 0,
        text: "line one",
        words: null,
      },
      {
        time_ms: 0,
        text: "line two",
        words: null,
      },
    ];

    const markup = renderToStaticMarkup(<LyricsPanel />);

    expect(markup).toContain('text-white">line one</span>');
    expect(markup).toContain('text-white">line two</span>');
    expect(markup).not.toContain('text-[var(--color-active)]">line one</span>');
  });

  test("uses fullscreen audience layout without edit chrome when requested", () => {
    const markup = renderToStaticMarkup(
      <LyricsPanel presentation="audience" />,
    );

    expect(markup).toContain("max-w-[min(92vw,1600px)]");
    expect(markup).toContain("min-h-full");
    expect(markup).not.toContain("contextual-reveal absolute right-4 top-4");
    expect(markup).not.toContain("absolute inset-x-0 bottom-0");
  });

  test("renders stable line markers for timed lyrics auto-scroll targeting", () => {
    mockLyricsState.lines = [
      {
        time_ms: 1000,
        text: "line one",
        words: null,
      },
      {
        time_ms: 2000,
        text: "line two",
        words: null,
      },
    ];
    mockLyricsState.activeLineIndex = 1;

    const markup = renderToStaticMarkup(<LyricsPanel />);

    expect(markup).toContain('data-lyrics-line-index="0"');
    expect(markup).toContain('data-lyrics-line-index="1"');
  });
});
