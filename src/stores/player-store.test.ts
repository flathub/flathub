import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";
import type { PlaybackPositionEvent, PlaybackStateSnapshot } from "@/types/ipc";
import {
  createPlayerStore,
  DEFAULT_AIRPLAY_OUTPUT_STATE,
  selectCurrentPositionMs,
  selectSyncDisplayPositionMs,
  type PlayerSyncSnapshot,
  usePlayerStore,
} from "./player-store";

interface FakeChannel {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
}

function playbackSnapshot(
  overrides: Partial<PlaybackStateSnapshot> = {},
): PlaybackStateSnapshot {
  return {
    song_id: "song-1",
    state: "playing",
    is_playing: true,
    position_ms: 1200,
    duration_ms: 3000,
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

function playbackPositionEvent(
  snapshot: PlaybackStateSnapshot,
): PlaybackPositionEvent {
  return {
    ms: snapshot.position_ms,
    snapshot,
  };
}

describe("selectSyncDisplayPositionMs", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    usePlayerStore.setState({
      airPlayPlainTextPagePending: false,
      airPlayPlainTextPagePendingDirection: null,
    });
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  test("prefers the AirPlay displayed position when active", () => {
    expect(
      selectSyncDisplayPositionMs({
        positionMs: 1000,
        airPlayOutput: {
          active: true,
          audioActive: true,
          routeName: "Living Room TV",
          mode: "lyrics",
          phase: "playing",
          detail: null,
          displayedPositionMs: 1250,
          streamGeneration: 7,
          latencyMs: 200,
        },
      }),
    ).toBe(1250);
  });

  test("falls back to the local playback position when AirPlay is inactive", () => {
    expect(
      selectSyncDisplayPositionMs({
        positionMs: 1000,
        airPlayOutput: {
          active: false,
          audioActive: false,
          routeName: null,
          mode: "idle",
          phase: "idle",
          detail: null,
          displayedPositionMs: 1250,
          streamGeneration: 0,
          latencyMs: null,
        },
      }),
    ).toBe(1000);
  });

  test("clears AirPlay plain-text page feedback after the lock window elapses", () => {
    usePlayerStore.getState().startAirPlayPlainTextPagePending("prev", 900);

    expect(usePlayerStore.getState().airPlayPlainTextPagePending).toBe(true);
    expect(usePlayerStore.getState().airPlayPlainTextPagePendingDirection).toBe(
      "prev",
    );

    vi.advanceTimersByTime(900);

    expect(usePlayerStore.getState().airPlayPlainTextPagePending).toBe(false);
    expect(usePlayerStore.getState().airPlayPlainTextPagePendingDirection).toBe(
      null,
    );
  });

  describe("selectCurrentPositionMs", () => {
    test("returns positionMs when playback is paused", () => {
      expect(
        selectCurrentPositionMs(
          {
            snapshot: playbackSnapshot({ is_playing: false }),
            positionMs: 1500,
            playingSinceMs: null,
          },
          () => 2000,
        ),
      ).toBe(1500);
    });

    test("returns positionMs when no snapshot exists", () => {
      expect(
        selectCurrentPositionMs(
          {
            snapshot: null,
            positionMs: 0,
            playingSinceMs: null,
          },
          () => 5000,
        ),
      ).toBe(0);
    });

    test("extrapolates position from the last sync point when playing", () => {
      expect(
        selectCurrentPositionMs(
          {
            snapshot: playbackSnapshot({ position_ms: 1200, is_playing: true }),
            positionMs: 1200,
            playingSinceMs: 1000,
          },
          () => 1500,
        ),
      ).toBe(1700);
    });

    test("advances smoothly between position events", () => {
      // Initial play at position 0, synced at monotonic time 1000
      expect(
        selectCurrentPositionMs(
          {
            snapshot: playbackSnapshot({ position_ms: 0, is_playing: true }),
            positionMs: 0,
            playingSinceMs: 1000,
          },
          () => 1000,
        ),
      ).toBe(0);

      // 33 ms later, position event arrives
      expect(
        selectCurrentPositionMs(
          {
            snapshot: playbackSnapshot({ position_ms: 33, is_playing: true }),
            positionMs: 33,
            playingSinceMs: 1033,
          },
          () => 1050,
        ),
      ).toBe(50);

      // 33 ms later, next position event
      expect(
        selectCurrentPositionMs(
          {
            snapshot: playbackSnapshot({ position_ms: 66, is_playing: true }),
            positionMs: 66,
            playingSinceMs: 1066,
          },
          () => 1066,
        ),
      ).toBe(66);
    });

    test("continues advancing even without position events arriving", () => {
      // Play at position 0, synced at monotonic time 1000
      // 500 ms passes, no events arrive
      expect(
        selectCurrentPositionMs(
          {
            snapshot: playbackSnapshot({ position_ms: 0, is_playing: true }),
            positionMs: 0,
            playingSinceMs: 1000,
          },
          () => 1500,
        ),
      ).toBe(500);
    });
  });

  test("syncs playback snapshot and position across webview contexts", () => {
    const channelsByName = new Map<string, Set<FakeChannel>>();
    const channelFactory = (name: string) => {
      const peers = channelsByName.get(name) ?? new Set<FakeChannel>();
      channelsByName.set(name, peers);

      const channel: FakeChannel = {
        onmessage: null,
        postMessage(data: unknown) {
          for (const peer of peers) {
            if (peer === channel) {
              continue;
            }

            peer.onmessage?.({ data });
          }
        },
        close() {
          peers.delete(channel);
        },
      };

      peers.add(channel);
      return channel;
    };

    const primary = createPlayerStore(
      createWebviewSyncChannel<PlayerSyncSnapshot>("player", {
        channelFactory,
        originId: "primary",
      }),
    );
    const secondary = createPlayerStore(
      createWebviewSyncChannel<PlayerSyncSnapshot>("player", {
        channelFactory,
        originId: "secondary",
      }),
    );

    primary.store.getState().updateSnapshot(playbackSnapshot());
    primary.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1500 })),
      );

    expect(secondary.store.getState().snapshot?.song_id).toBe("song-1");
    expect(secondary.store.getState().positionMs).toBe(1500);
    expect(secondary.store.getState().airPlayOutput).toEqual(
      DEFAULT_AIRPLAY_OUTPUT_STATE,
    );

    primary.dispose();
    secondary.dispose();
  });

  test("applies the authoritative playback snapshot when background loading starts playback", () => {
    const player = createPlayerStore();
    player.store.getState().updateSnapshot(
      playbackSnapshot({
        state: "loading",
        is_playing: false,
        position_ms: 0,
        duration_ms: null,
      }),
    );

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1200 })),
      );

    expect(player.store.getState().snapshot).toEqual(
      playbackSnapshot({ position_ms: 1200 }),
    );
    expect(player.store.getState().positionMs).toBe(1200);

    player.dispose();
  });

  test("keeps the snapshot stable for ordinary position ticks", () => {
    const player = createPlayerStore();
    const currentSnapshot = playbackSnapshot();
    player.store.getState().updateSnapshot(currentSnapshot);

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1500 })),
      );

    expect(player.store.getState().snapshot).toBe(currentSnapshot);
    expect(player.store.getState().positionMs).toBe(1500);

    player.dispose();
  });

  test("advances positionMs across multiple ordinary position ticks", () => {
    const player = createPlayerStore();
    const currentSnapshot = playbackSnapshot();
    player.store.getState().updateSnapshot(currentSnapshot);

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1200 })),
      );
    expect(player.store.getState().positionMs).toBe(1200);

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1500 })),
      );
    expect(player.store.getState().positionMs).toBe(1500);

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1800 })),
      );
    expect(player.store.getState().positionMs).toBe(1800);

    player.dispose();
  });

  test("positions from the empty store via a series of position events", () => {
    const player = createPlayerStore();

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 0 })),
      );
    expect(player.store.getState().positionMs).toBe(0);
    expect(player.store.getState().snapshot).toBeDefined();

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 500 })),
      );
    expect(player.store.getState().positionMs).toBe(500);

    player.store
      .getState()
      .applyPlaybackPositionEvent(
        playbackPositionEvent(playbackSnapshot({ position_ms: 1500 })),
      );
    expect(player.store.getState().positionMs).toBe(1500);

    player.dispose();
  });
});
