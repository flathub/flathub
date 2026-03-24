import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";
import {
  createPlayerStore,
  DEFAULT_AIRPLAY_OUTPUT_STATE,
  selectSyncDisplayPositionMs,
  type PlayerSyncSnapshot,
  usePlayerStore,
} from "./player-store";

interface FakeChannel {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
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

    primary.store.getState().updateSnapshot({
      song_id: "song-1",
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
    });
    primary.store.getState().updatePosition(1500);

    expect(secondary.store.getState().snapshot?.song_id).toBe("song-1");
    expect(secondary.store.getState().positionMs).toBe(1500);
    expect(secondary.store.getState().airPlayOutput).toEqual(
      DEFAULT_AIRPLAY_OUTPUT_STATE,
    );

    primary.dispose();
    secondary.dispose();
  });
});
