import { create } from "zustand";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import {
  createWebviewSyncChannel,
  type WebviewSyncChannel,
} from "@/runtime/webview-sync";
import { useLibraryStore } from "@/stores/library-store";
import { useQueueStore } from "@/stores/queue-store";
import type {
  AirPlayOutputStateEvent,
  PlaybackStateSnapshot,
  StemName,
} from "@/types/ipc";
import {
  playTrackWithOptionalStems,
  shouldEnqueueInsteadOfReplacingCurrentSong,
} from "./player-workflows";

export const DEFAULT_AIRPLAY_OUTPUT_STATE: AirPlayOutputStateEvent = {
  active: false,
  audioActive: false,
  routeName: null,
  mode: "idle",
  phase: "idle",
  detail: null,
  displayedPositionMs: null,
  streamGeneration: 0,
  latencyMs: null,
};

interface PlayerState {
  snapshot: PlaybackStateSnapshot | null;
  positionMs: number;
  airPlayOutput: AirPlayOutputStateEvent;
  localAudienceOutputActive: boolean;
  airPlayPlainTextPagePending: boolean;
  airPlayPlainTextPagePendingDirection: "prev" | "next" | null;

  playSong: (songId: string) => Promise<void>;
  playNow: (songId: string) => Promise<void>;
  resume: () => Promise<void>;
  pause: () => Promise<void>;
  seek: (ms: number) => Promise<void>;
  setVolume: (level: number) => Promise<void>;
  setStemVolume: (stem: StemName, level: number) => Promise<void>;
  loadStems: () => Promise<void>;
  updatePosition: (ms: number) => void;
  updateSnapshot: (snapshot: PlaybackStateSnapshot) => void;
  loadState: () => Promise<void>;
  playNextFromQueue: (endedSongId: string) => Promise<void>;
  skipForward: () => Promise<void>;
  skipBack: () => Promise<void>;
  updateAirPlayOutput: (airPlayOutput: AirPlayOutputStateEvent) => void;
  updateLocalAudienceOutputActive: (active: boolean) => void;
  startAirPlayPlainTextPagePending: (
    direction: "prev" | "next",
    lockMs: number,
  ) => void;
  clearAirPlayPlainTextPagePending: () => void;
}

export interface PlayerSyncSnapshot {
  snapshot: PlaybackStateSnapshot | null;
  positionMs: number;
  airPlayOutput: AirPlayOutputStateEvent;
  localAudienceOutputActive: boolean;
  airPlayPlainTextPagePending: boolean;
  airPlayPlainTextPagePendingDirection: "prev" | "next" | null;
}

function createPlayerSyncSnapshot(state: PlayerState): PlayerSyncSnapshot {
  return {
    snapshot: state.snapshot,
    positionMs: state.positionMs,
    airPlayOutput: state.airPlayOutput,
    localAudienceOutputActive: state.localAudienceOutputActive,
    airPlayPlainTextPagePending: state.airPlayPlainTextPagePending,
    airPlayPlainTextPagePendingDirection:
      state.airPlayPlainTextPagePendingDirection,
  };
}

function applyPlayerSyncSnapshot(
  set: (partial: Partial<PlayerState>) => void,
  payload: PlayerSyncSnapshot,
) {
  set({
    snapshot: payload.snapshot,
    positionMs: payload.positionMs,
    airPlayOutput: payload.airPlayOutput,
    localAudienceOutputActive: payload.localAudienceOutputActive,
    airPlayPlainTextPagePending: payload.airPlayPlainTextPagePending,
    airPlayPlainTextPagePendingDirection:
      payload.airPlayPlainTextPagePendingDirection,
  });
}

// RATIONALE: Once AirPlay is active, the audience surface must follow the TV's
// displayed clock rather than the local playback clock. That keeps the
// standard UI synchronized with the remote audience surface without changing
// which window is allowed to render audience styling.
export function selectSyncDisplayPositionMs(
  state: Pick<PlayerState, "positionMs" | "airPlayOutput">,
): number {
  return state.airPlayOutput.active &&
    state.airPlayOutput.displayedPositionMs !== null
    ? state.airPlayOutput.displayedPositionMs
    : state.positionMs;
}

export function createPlayerStore(
  syncChannel: WebviewSyncChannel<PlayerSyncSnapshot> = createWebviewSyncChannel<PlayerSyncSnapshot>(
    "openkara.player",
  ),
) {
  let airPlayPlainTextPagePendingTimer: ReturnType<typeof setTimeout> | null =
    null;

  const store = create<PlayerState>((set, get) => {
    const syncPatch = (patch: Partial<PlayerState>) => {
      set(patch);
      syncChannel.publish(createPlayerSyncSnapshot(get()));
    };

    return {
      snapshot: null,
      positionMs: 0,
      airPlayOutput: DEFAULT_AIRPLAY_OUTPUT_STATE,
      localAudienceOutputActive: false,
      airPlayPlainTextPagePending: false,
      airPlayPlainTextPagePendingDirection: null,

      playSong: async (songId) => {
        const { snapshot } = get();
        if (shouldEnqueueInsteadOfReplacingCurrentSong(snapshot, songId)) {
          useQueueStore.getState().addToQueue(songId);
          return;
        }

        try {
          await playTrackWithOptionalStems(songId, {
            play: api.play,
            loadStems: api.loadStems,
            getSeparationStatus: (nextSongId) =>
              useLibraryStore.getState().separationStatuses[nextSongId],
            applySnapshot: (nextSnapshot) =>
              syncPatch({
                snapshot: nextSnapshot,
                positionMs: nextSnapshot.position_ms,
              }),
          });
        } catch (e) {
          notifyError(e, () => get().playSong(songId));
        }
      },

      playNow: async (songId) => {
        try {
          await playTrackWithOptionalStems(songId, {
            play: api.play,
            loadStems: api.loadStems,
            getSeparationStatus: (nextSongId) =>
              useLibraryStore.getState().separationStatuses[nextSongId],
            applySnapshot: (nextSnapshot) =>
              syncPatch({
                snapshot: nextSnapshot,
                positionMs: nextSnapshot.position_ms,
              }),
          });
        } catch (e) {
          notifyError(e, () => get().playNow(songId));
        }
      },

      resume: async () => {
        try {
          const snapshot = await api.resume();
          syncPatch({ snapshot, positionMs: snapshot.position_ms });
        } catch (e) {
          notifyError(e);
        }
      },

      pause: async () => {
        try {
          const snapshot = await api.pause();
          syncPatch({ snapshot, positionMs: snapshot.position_ms });
        } catch (e) {
          notifyError(e);
        }
      },

      seek: async (ms) => {
        const current = get().snapshot;
        if (!current?.song_id) return;
        try {
          const clamped = Math.max(0, ms);
          const snapshot = await api.seek(clamped);
          syncPatch({ snapshot, positionMs: snapshot.position_ms });
        } catch (e) {
          notifyError(e);
        }
      },

      setVolume: async (level) => {
        try {
          const clamped = Math.max(0, Math.min(1, level));
          const snapshot = await api.setVolume(clamped);
          syncPatch({ snapshot });
        } catch (e) {
          notifyError(e);
        }
      },

      setStemVolume: async (stem, level) => {
        try {
          const clamped = Math.max(0, Math.min(1, level));
          const snapshot = await api.setStemVolume(stem, clamped);
          syncPatch({ snapshot });
        } catch (e) {
          notifyError(e);
        }
      },

      loadStems: async () => {
        try {
          const snapshot = await api.loadStems();
          syncPatch({ snapshot });
        } catch (e) {
          notifyError(e, () => get().loadStems());
        }
      },

      updatePosition: (ms) => {
        const { snapshot } = get();
        if (snapshot && !snapshot.is_playing) return;
        syncPatch({ positionMs: ms });
      },

      updateSnapshot: (snapshot) => {
        syncPatch({ snapshot, positionMs: snapshot.position_ms });
      },

      loadState: async () => {
        try {
          const snapshot = await api.getPlaybackState();
          syncPatch({ snapshot, positionMs: snapshot.position_ms });
        } catch (e) {
          notifyError(e);
        }
      },

      playNextFromQueue: async (endedSongId) => {
        const { snapshot } = get();
        if (snapshot?.song_id !== endedSongId) return;

        const nextId = useQueueStore.getState().dequeue();
        if (!nextId) return;

        try {
          await playTrackWithOptionalStems(nextId, {
            play: api.play,
            loadStems: api.loadStems,
            getSeparationStatus: (nextSongId) =>
              useLibraryStore.getState().separationStatuses[nextSongId],
            applySnapshot: (nextSnapshot) =>
              syncPatch({
                snapshot: nextSnapshot,
                positionMs: nextSnapshot.position_ms,
              }),
          });
        } catch (e) {
          notifyError(e);
        }
      },

      skipForward: async () => {
        const nextId = useQueueStore.getState().dequeue();
        if (!nextId) return;

        try {
          await playTrackWithOptionalStems(nextId, {
            play: api.play,
            loadStems: api.loadStems,
            getSeparationStatus: (nextSongId) =>
              useLibraryStore.getState().separationStatuses[nextSongId],
            applySnapshot: (nextSnapshot) =>
              syncPatch({
                snapshot: nextSnapshot,
                positionMs: nextSnapshot.position_ms,
              }),
          });
        } catch (e) {
          notifyError(e);
        }
      },

      skipBack: async () => {
        const { positionMs, snapshot } = get();
        if (!snapshot?.song_id) return;

        if (positionMs > 3000) {
          try {
            const newSnapshot = await api.seek(0);
            syncPatch({
              snapshot: newSnapshot,
              positionMs: newSnapshot.position_ms,
            });
          } catch (e) {
            notifyError(e);
          }
        }
      },

      updateAirPlayOutput: (airPlayOutput) => {
        syncPatch({ airPlayOutput });
      },

      updateLocalAudienceOutputActive: (active) => {
        syncPatch({ localAudienceOutputActive: active });
      },

      startAirPlayPlainTextPagePending: (direction, lockMs) => {
        if (airPlayPlainTextPagePendingTimer !== null) {
          clearTimeout(airPlayPlainTextPagePendingTimer);
        }

        syncPatch({
          airPlayPlainTextPagePending: true,
          airPlayPlainTextPagePendingDirection: direction,
        });

        airPlayPlainTextPagePendingTimer = setTimeout(() => {
          airPlayPlainTextPagePendingTimer = null;
          get().clearAirPlayPlainTextPagePending();
        }, lockMs);
      },

      clearAirPlayPlainTextPagePending: () => {
        if (airPlayPlainTextPagePendingTimer !== null) {
          clearTimeout(airPlayPlainTextPagePendingTimer);
          airPlayPlainTextPagePendingTimer = null;
        }

        syncPatch({
          airPlayPlainTextPagePending: false,
          airPlayPlainTextPagePendingDirection: null,
        });
      },
    };
  });

  const unsubscribe = syncChannel.subscribe((payload) => {
    applyPlayerSyncSnapshot(store.setState, payload);
  });

  return {
    store,
    dispose() {
      if (airPlayPlainTextPagePendingTimer !== null) {
        clearTimeout(airPlayPlainTextPagePendingTimer);
        airPlayPlainTextPagePendingTimer = null;
      }
      unsubscribe();
      syncChannel.close();
    },
  };
}

const defaultPlayerStore = createPlayerStore();

export const usePlayerStore = defaultPlayerStore.store;
