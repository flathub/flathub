import { describe, expect, test, vi } from "vitest";
import type {
  PlaybackStateSnapshot,
  SeparationStatusSnapshot,
} from "@/types/ipc";
import {
  playSongWithOptionalStems,
  shouldEnqueueInsteadOfReplacingCurrentSong,
  shouldLoadSeparatedStems,
} from "./player-workflows";

function playbackSnapshot(
  overrides: Partial<PlaybackStateSnapshot> = {},
): PlaybackStateSnapshot {
  return {
    song_id: null,
    state: "idle",
    is_playing: false,
    position_ms: 0,
    duration_ms: null,
    volume: 1,
    stem_volumes: {
      vocals: 1,
      drums: 1,
      bass: 1,
      other: 1,
    },
    has_stems: false,
    stem_mode: null,
    ...overrides,
  };
}

function completedSeparationStatus(
  overrides: Partial<SeparationStatusSnapshot> = {},
): SeparationStatusSnapshot {
  return {
    song_id: "song-1",
    state: "completed",
    percent: 100,
    cache_hit: true,
    vocals_path: "vocals.ogg",
    accomp_path: "accomp.ogg",
    drums_path: null,
    bass_path: null,
    other_path: null,
    model_variant: "htdemucs",
    error: null,
    ...overrides,
  };
}

describe("player-workflows", () => {
  test("queues instead of replacing when another song is already playing", () => {
    expect(
      shouldEnqueueInsteadOfReplacingCurrentSong(
        playbackSnapshot({ song_id: "current", is_playing: true }),
        "next-song",
      ),
    ).toBe(true);
  });

  test("does not queue when replaying the current song", () => {
    expect(
      shouldEnqueueInsteadOfReplacingCurrentSong(
        playbackSnapshot({ song_id: "current", is_playing: true }),
        "current",
      ),
    ).toBe(false);
  });

  test("loads stems only when separation is completed and playback is still on original audio", () => {
    expect(
      shouldLoadSeparatedStems(
        playbackSnapshot({ has_stems: false }),
        completedSeparationStatus(),
      ),
    ).toBe(true);
    expect(
      shouldLoadSeparatedStems(
        playbackSnapshot({ has_stems: true }),
        completedSeparationStatus(),
      ),
    ).toBe(false);
    expect(
      shouldLoadSeparatedStems(
        playbackSnapshot({ state: "loading", has_stems: false }),
        completedSeparationStatus(),
      ),
    ).toBe(false);
    expect(
      shouldLoadSeparatedStems(
        playbackSnapshot({ has_stems: false }),
        completedSeparationStatus({ state: "running" }),
      ),
    ).toBe(false);
  });

  test("does not load stems while remote playback is still loading", async () => {
    const play = vi.fn().mockResolvedValue(
      playbackSnapshot({
        song_id: "song-remote",
        state: "loading",
        is_playing: false,
        has_stems: false,
      }),
    );
    const loadStems = vi.fn();
    const applySnapshot = vi.fn();

    await playSongWithOptionalStems("song-remote", {
      play,
      loadStems,
      getSeparationStatus: () => completedSeparationStatus(),
      applySnapshot,
    });

    expect(loadStems).not.toHaveBeenCalled();
    expect(applySnapshot).toHaveBeenCalledTimes(1);
  });

  test("plays the requested song and then loads cached stems when available", async () => {
    const play = vi.fn().mockResolvedValue(
      playbackSnapshot({
        song_id: "song-1",
        is_playing: true,
        has_stems: false,
      }),
    );
    const loadStems = vi.fn().mockResolvedValue(
      playbackSnapshot({
        song_id: "song-1",
        is_playing: true,
        has_stems: true,
      }),
    );
    const applySnapshot = vi.fn();

    await playSongWithOptionalStems("song-1", {
      play,
      loadStems,
      getSeparationStatus: () => completedSeparationStatus(),
      applySnapshot,
    });

    expect(play).toHaveBeenCalledWith("song-1");
    expect(loadStems).toHaveBeenCalledTimes(1);
    expect(applySnapshot).toHaveBeenNthCalledWith(
      1,
      expect.objectContaining({ song_id: "song-1", has_stems: false }),
    );
    expect(applySnapshot).toHaveBeenNthCalledWith(
      2,
      expect.objectContaining({ song_id: "song-1", has_stems: true }),
    );
  });

  test("skips stem loading when no completed separation is cached", async () => {
    const play = vi.fn().mockResolvedValue(
      playbackSnapshot({
        song_id: "song-2",
        is_playing: true,
        has_stems: false,
      }),
    );
    const loadStems = vi.fn();
    const applySnapshot = vi.fn();

    await playSongWithOptionalStems("song-2", {
      play,
      loadStems,
      getSeparationStatus: () => undefined,
      applySnapshot,
    });

    expect(loadStems).not.toHaveBeenCalled();
    expect(applySnapshot).toHaveBeenCalledTimes(1);
  });
});
