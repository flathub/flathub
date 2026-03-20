import { useEffect } from "react";
import { listen } from "@tauri-apps/api/event";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { closeFullscreenPlayer } from "@/lib/fullscreen-player";
import { songHasCdgMedia } from "@/lib/song-media";
import * as api from "@/lib/tauri";
import { useCdgStore } from "@/stores/cdg-store";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { usePlayerStore } from "@/stores/player-store";
import { useSettingsStore } from "@/stores/settings-store";
import type {
  AirPlayAudienceStatePayload,
  AirPlayOutputStateEvent,
  PlaybackStateSnapshot,
} from "@/types/ipc";

const AIRPLAY_OUTPUT_STATE_EVENT = "openkara://airplay-output-state";

interface BuildAirPlayAudienceStateOptions {
  playbackSnapshot: PlaybackStateSnapshot | null;
  positionMs: number;
  lyricsSongId: string | null;
  lines: AirPlayAudienceStatePayload["lines"];
  activeLineIndex: number;
  offsetMs: number;
  lyricsFontStep: number;
  hasCdg: boolean;
  currentSongHasCdg: boolean;
}

export function buildAirPlayAudienceState({
  playbackSnapshot,
  positionMs,
  lyricsSongId,
  lines,
  activeLineIndex,
  offsetMs,
  lyricsFontStep,
  hasCdg,
  currentSongHasCdg,
}: BuildAirPlayAudienceStateOptions): AirPlayAudienceStatePayload {
  const songId = playbackSnapshot?.song_id ?? null;

  if (!songId) {
    return {
      mode: "idle",
      songId: null,
      isPlaying: false,
      positionMs: 0,
      lines: [],
      activeLineIndex: -1,
      offsetMs: 0,
      lyricsFontStep,
    };
  }

  if (hasCdg || currentSongHasCdg) {
    return {
      mode: "cdg",
      songId,
      isPlaying: playbackSnapshot?.is_playing ?? false,
      positionMs,
      lines: [],
      activeLineIndex: -1,
      offsetMs: 0,
      lyricsFontStep,
    };
  }

  return {
    mode: "lyrics",
    songId: lyricsSongId ?? songId,
    isPlaying: playbackSnapshot?.is_playing ?? false,
    positionMs,
    lines,
    activeLineIndex,
    offsetMs,
    lyricsFontStep,
  };
}

export function useAirPlayAudienceSync(enabled = true): void {
  const playbackSnapshot = usePlayerStore((s) => s.snapshot);
  const positionMs = usePlayerStore((s) => s.positionMs);
  const lyricsSongId = useLyricsStore((s) => s.songId);
  const lines = useLyricsStore((s) => s.lines);
  const activeLineIndex = useLyricsStore((s) => s.activeLineIndex);
  const offsetMs = useLyricsStore((s) => s.offsetMs);
  const hasCdg = useCdgStore((s) => s.hasCdg);
  const songs = useLibraryStore((s) => s.songs);
  const lyricsFontStep = useSettingsStore((s) => s.lyricsFontStep);

  const currentSongHasCdg = songHasCdgMedia(
    songs.find((song) => song.hash === playbackSnapshot?.song_id) ?? null,
  );
  const isMac = getShortcutPlatform() === "mac";

  useEffect(() => {
    if (!enabled || !isMac) {
      return;
    }

    void api
      .syncAirPlayAudienceState(
        buildAirPlayAudienceState({
          playbackSnapshot,
          positionMs,
          lyricsSongId,
          lines,
          activeLineIndex,
          offsetMs,
          lyricsFontStep,
          hasCdg,
          currentSongHasCdg,
        }),
      )
      .catch(() => {
        // Auxiliary sync failures must not interrupt local playback.
      });
  }, [
    activeLineIndex,
    currentSongHasCdg,
    enabled,
    hasCdg,
    isMac,
    lines,
    lyricsFontStep,
    lyricsSongId,
    offsetMs,
    playbackSnapshot,
    positionMs,
  ]);
}

export function useAirPlayOutputState(enabled = true): void {
  const isMac = getShortcutPlatform() === "mac";

  useEffect(() => {
    if (!enabled || !isMac) {
      return;
    }

    let cancelled = false;
    let unlisten: (() => void) | null = null;

    const setup = async () => {
      unlisten = await listen<AirPlayOutputStateEvent>(
        AIRPLAY_OUTPUT_STATE_EVENT,
        (event) => {
          if (cancelled || !event.payload.active) {
            return;
          }

          void closeFullscreenPlayer();
        },
      );
    };

    void setup();

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [enabled, isMac]);
}
