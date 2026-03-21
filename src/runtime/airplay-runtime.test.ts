import { describe, expect, test } from "vitest";
import { buildAirPlayAudienceState } from "./airplay-runtime";

describe("buildAirPlayAudienceState", () => {
  test("returns idle when no song is loaded", () => {
    expect(
      buildAirPlayAudienceState({
        playbackSnapshot: null,
        lyricsSongId: null,
        lines: [],
        offsetMs: 0,
        isLoading: false,
        lyricsFontStep: 0,
        hasCdg: false,
        currentSongHasCdg: false,
        messages: {
          selectSong: "Select a song to start",
          loadingLyrics: "Loading lyrics...",
          noLyrics: "No lyrics available for this track",
          addLyrics: "Add Lyrics",
        },
      }),
    ).toEqual({
      mode: "idle",
      songId: null,
      lines: [],
      offsetMs: 0,
      isLoading: false,
      lyricsFontStep: 0,
      messages: {
        selectSong: "Select a song to start",
        loadingLyrics: "Loading lyrics...",
        noLyrics: "No lyrics available for this track",
        addLyrics: "Add Lyrics",
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
      presentationSpec: {
        contentWidthRatio: 0.92,
        contentMaxWidthPx: 1600,
        horizontalPaddingPx: 64,
        verticalPaddingPx: 56,
        lineGapPx: 40,
        fontSizePx: 72,
        lineHeightMultiple: 1.08,
        activeScale: 1.05,
        statusFontSizePx: 18,
        activeGlowBlurPx: 12,
        activeTextColor: { red: 1, green: 1, blue: 1, alpha: 1 },
        pastTextColor: {
          red: 72 / 255,
          green: 72 / 255,
          blue: 74 / 255,
          alpha: 1,
        },
        futureTextColor: {
          red: 58 / 255,
          green: 58 / 255,
          blue: 60 / 255,
          alpha: 1,
        },
        plainTextColor: { red: 1, green: 1, blue: 1, alpha: 1 },
        statusTextColor: {
          red: 142 / 255,
          green: 142 / 255,
          blue: 147 / 255,
          alpha: 1,
        },
        activeGlowColor: { red: 1, green: 1, blue: 1, alpha: 0.8 },
      },
    });
  });

  test("returns lyrics payload when the current song is lyric-driven", () => {
    expect(
      buildAirPlayAudienceState({
        playbackSnapshot: {
          song_id: "song-1",
          is_playing: true,
          position_ms: 1234,
          duration_ms: 5000,
          volume: 1,
          stem_volumes: {
            vocals: 1,
            drums: 1,
            bass: 1,
            other: 1,
          },
          has_stems: false,
          stem_mode: null,
        },
        lyricsSongId: "song-1",
        lines: [{ time_ms: 1200, text: "Hello", words: null }],
        offsetMs: 100,
        isLoading: false,
        lyricsFontStep: 2,
        hasCdg: false,
        currentSongHasCdg: false,
        messages: {
          selectSong: "Select a song to start",
          loadingLyrics: "Loading lyrics...",
          noLyrics: "No lyrics available for this track",
          addLyrics: "Add Lyrics",
        },
      }),
    ).toMatchObject({
      mode: "lyrics",
      songId: "song-1",
      lines: [{ time_ms: 1200, text: "Hello", words: null }],
      offsetMs: 100,
      isLoading: false,
      lyricsFontStep: 2,
      messages: {
        selectSong: "Select a song to start",
        loadingLyrics: "Loading lyrics...",
        noLyrics: "No lyrics available for this track",
        addLyrics: "Add Lyrics",
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
      presentationSpec: {
        fontSizePx: 96,
        contentWidthRatio: 0.92,
        contentMaxWidthPx: 1600,
      },
    });
  });

  test("returns cdg mode whenever the active song should render CDG", () => {
    expect(
      buildAirPlayAudienceState({
        playbackSnapshot: {
          song_id: "song-2",
          is_playing: false,
          position_ms: 900,
          duration_ms: 5000,
          volume: 0.8,
          stem_volumes: {
            vocals: 1,
            drums: 0.8,
            bass: 0.8,
            other: 0.8,
          },
          has_stems: true,
          stem_mode: "two_stem",
        },
        lyricsSongId: "song-2",
        lines: [{ time_ms: 0, text: "Ignored", words: null }],
        offsetMs: 0,
        isLoading: false,
        lyricsFontStep: 1,
        hasCdg: true,
        currentSongHasCdg: true,
        messages: {
          selectSong: "Select a song to start",
          loadingLyrics: "Loading lyrics...",
          noLyrics: "No lyrics available for this track",
          addLyrics: "Add Lyrics",
        },
      }),
    ).toMatchObject({
      mode: "cdg",
      songId: "song-2",
      lines: [],
      offsetMs: 0,
      isLoading: false,
      lyricsFontStep: 1,
      messages: {
        selectSong: "Select a song to start",
        loadingLyrics: "Loading lyrics...",
        noLyrics: "No lyrics available for this track",
        addLyrics: "Add Lyrics",
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
      presentationSpec: {
        fontSizePx: 96,
      },
    });
  });

  test("preserves loading and empty-state copy for native audience rendering", () => {
    expect(
      buildAirPlayAudienceState({
        playbackSnapshot: {
          song_id: "song-3",
          is_playing: false,
          position_ms: 0,
          duration_ms: 5000,
          volume: 1,
          stem_volumes: {
            vocals: 1,
            drums: 1,
            bass: 1,
            other: 1,
          },
          has_stems: false,
          stem_mode: null,
        },
        lyricsSongId: "song-3",
        lines: [],
        offsetMs: 0,
        isLoading: true,
        lyricsFontStep: 0,
        hasCdg: false,
        currentSongHasCdg: false,
        messages: {
          selectSong: "选择一首歌曲开始",
          loadingLyrics: "正在加载歌词...",
          noLyrics: "这首歌暂无歌词",
          addLyrics: "添加歌词",
        },
      }),
    ).toMatchObject({
      mode: "lyrics",
      songId: "song-3",
      lines: [],
      offsetMs: 0,
      isLoading: true,
      lyricsFontStep: 0,
      messages: {
        selectSong: "选择一首歌曲开始",
        loadingLyrics: "正在加载歌词...",
        noLyrics: "这首歌暂无歌词",
        addLyrics: "添加歌词",
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
      presentationSpec: {
        statusFontSizePx: 18,
      },
    });
  });

  test("drops stale lyrics lines until they belong to the current song", () => {
    expect(
      buildAirPlayAudienceState({
        playbackSnapshot: {
          song_id: "song-4",
          is_playing: true,
          position_ms: 2048,
          duration_ms: 5000,
          volume: 1,
          stem_volumes: {
            vocals: 1,
            drums: 1,
            bass: 1,
            other: 1,
          },
          has_stems: false,
          stem_mode: null,
        },
        lyricsSongId: "song-3",
        lines: [{ time_ms: 1200, text: "stale", words: null }],
        offsetMs: 0,
        isLoading: false,
        lyricsFontStep: 0,
        hasCdg: false,
        currentSongHasCdg: false,
        messages: {
          selectSong: "Select a song to start",
          loadingLyrics: "Loading lyrics...",
          noLyrics: "No lyrics available for this track",
          addLyrics: "Add Lyrics",
        },
      }),
    ).toMatchObject({
      mode: "lyrics",
      songId: "song-4",
      lines: [],
      offsetMs: 0,
      isLoading: true,
      lyricsFontStep: 0,
      messages: {
        selectSong: "Select a song to start",
        loadingLyrics: "Loading lyrics...",
        noLyrics: "No lyrics available for this track",
        addLyrics: "Add Lyrics",
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
      presentationSpec: {
        fontSizePx: 72,
      },
    });
  });
});
