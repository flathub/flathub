import { useEffect } from "react";
import { listen } from "@tauri-apps/api/event";
import { useTranslation } from "react-i18next";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { buildAudiencePresentationSpec } from "@/lib/audience-presentation";
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
const AIRPLAY_VIEWPORT = {
  widthPx: 1280,
  heightPx: 720,
  bottomInsetPx: 0,
} as const;

interface BuildAirPlayAudienceStateOptions {
  playbackSnapshot: PlaybackStateSnapshot | null;
  lyricsSongId: string | null;
  lines: AirPlayAudienceStatePayload["lines"];
  offsetMs: number;
  isLoading: boolean;
  lyricsFontStep: number;
  hasCdg: boolean;
  currentSongHasCdg: boolean;
  messages: AirPlayAudienceStatePayload["messages"];
}

export function buildAirPlayAudienceState({
  playbackSnapshot,
  lyricsSongId,
  lines,
  offsetMs,
  isLoading,
  lyricsFontStep,
  hasCdg,
  currentSongHasCdg,
  messages,
}: BuildAirPlayAudienceStateOptions): AirPlayAudienceStatePayload {
  const songId = playbackSnapshot?.song_id ?? null;

  if (!songId) {
    return {
      mode: "idle",
      songId: null,
      lines: [],
      offsetMs: 0,
      isLoading,
      lyricsFontStep,
      messages,
      viewport: AIRPLAY_VIEWPORT,
      presentationSpec: buildAudiencePresentationSpec(lyricsFontStep),
    };
  }

  if (hasCdg || currentSongHasCdg) {
    return {
      mode: "cdg",
      songId,
      lines: [],
      offsetMs: 0,
      isLoading,
      lyricsFontStep,
      messages,
      viewport: AIRPLAY_VIEWPORT,
      presentationSpec: buildAudiencePresentationSpec(lyricsFontStep),
    };
  }

  const lyricsBelongToCurrentSong = lyricsSongId === songId;
  return {
    mode: "lyrics",
    songId,
    lines: lyricsBelongToCurrentSong ? lines : [],
    offsetMs,
    isLoading: isLoading || !lyricsBelongToCurrentSong,
    lyricsFontStep,
    messages,
    viewport: AIRPLAY_VIEWPORT,
    presentationSpec: buildAudiencePresentationSpec(lyricsFontStep),
  };
}

export function useAirPlayAudienceSync(enabled = true): void {
  const { t } = useTranslation();
  const playbackSnapshot = usePlayerStore((s) => s.snapshot);
  const lyricsSongId = useLyricsStore((s) => s.songId);
  const lines = useLyricsStore((s) => s.lines);
  const offsetMs = useLyricsStore((s) => s.offsetMs);
  const isLoading = useLyricsStore((s) => s.isLoading);
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
          lyricsSongId,
          lines,
          offsetMs,
          isLoading,
          lyricsFontStep,
          hasCdg,
          currentSongHasCdg,
          messages: {
            selectSong: t("lyrics.selectSong"),
            loadingLyrics: t("lyrics.loadingLyrics"),
            noLyrics: t("lyrics.noLyrics"),
            addLyrics: t("lyrics.addLyrics"),
          },
        }),
      )
      .catch(() => {
        // Auxiliary sync failures must not interrupt local playback.
      });
  }, [
    currentSongHasCdg,
    enabled,
    hasCdg,
    isMac,
    isLoading,
    lines,
    lyricsFontStep,
    lyricsSongId,
    offsetMs,
    playbackSnapshot,
    t,
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
          if (cancelled) {
            return;
          }

          usePlayerStore.getState().updateAirPlayOutput(event.payload);

          if (!event.payload.active || event.payload.phase !== "playing") {
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
