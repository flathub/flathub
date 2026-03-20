import { describe, expect, test } from "vitest";
import { buildAirPlayAudienceState } from "./airplay-runtime";

describe("buildAirPlayAudienceState", () => {
  test("returns idle when no song is loaded", () => {
    expect(
      buildAirPlayAudienceState({
        playbackSnapshot: null,
        positionMs: 0,
        lyricsSongId: null,
        lines: [],
        activeLineIndex: -1,
        offsetMs: 0,
        lyricsFontStep: 0,
        hasCdg: false,
        currentSongHasCdg: false,
      }),
    ).toEqual({
      mode: "idle",
      songId: null,
      isPlaying: false,
      positionMs: 0,
      lines: [],
      activeLineIndex: -1,
      offsetMs: 0,
      lyricsFontStep: 0,
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
        positionMs: 1400,
        lyricsSongId: "song-1",
        lines: [{ time_ms: 1200, text: "Hello", words: null }],
        activeLineIndex: 0,
        offsetMs: 100,
        lyricsFontStep: 2,
        hasCdg: false,
        currentSongHasCdg: false,
      }),
    ).toEqual({
      mode: "lyrics",
      songId: "song-1",
      isPlaying: true,
      positionMs: 1400,
      lines: [{ time_ms: 1200, text: "Hello", words: null }],
      activeLineIndex: 0,
      offsetMs: 100,
      lyricsFontStep: 2,
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
        positionMs: 900,
        lyricsSongId: "song-2",
        lines: [{ time_ms: 0, text: "Ignored", words: null }],
        activeLineIndex: 0,
        offsetMs: 0,
        lyricsFontStep: 1,
        hasCdg: true,
        currentSongHasCdg: true,
      }),
    ).toEqual({
      mode: "cdg",
      songId: "song-2",
      isPlaying: false,
      positionMs: 900,
      lines: [],
      activeLineIndex: -1,
      offsetMs: 0,
      lyricsFontStep: 1,
    });
  });
});
