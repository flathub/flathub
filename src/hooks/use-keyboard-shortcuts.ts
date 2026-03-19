import { useEffect } from "react";
import { usePlayerStore } from "@/stores/player-store";
import { useLayoutStore } from "@/stores/layout-store";
import { useSettingsStore } from "@/stores/settings-store";
import {
  APP_SHORTCUTS,
  isEditableShortcutTarget,
  matchesShortcut,
} from "@/lib/app-shortcuts";

interface KeyboardShortcutPlayerState {
  snapshot: ReturnType<typeof usePlayerStore.getState>["snapshot"];
  positionMs: number;
  resume: () => Promise<void>;
  pause: () => Promise<void>;
  seek: (ms: number) => Promise<void>;
  setVolume: (level: number) => Promise<void>;
}

interface KeyboardShortcutDeps {
  toggleSettings: () => void;
  toggleSidebar: () => void;
  adjustLyricsFont: (delta: number) => void;
  resetLyricsFont: () => void;
  player: KeyboardShortcutPlayerState;
}

export function handleAppKeyDown(
  e: KeyboardEvent,
  {
    toggleSettings,
    toggleSidebar,
    adjustLyricsFont,
    resetLyricsFont,
    player,
  }: KeyboardShortcutDeps,
): boolean {
  if (matchesShortcut(APP_SHORTCUTS.toggleSettings, e)) {
    e.preventDefault();
    toggleSettings();
    return true;
  }

  if (isEditableShortcutTarget(e.target)) {
    return false;
  }

  if (matchesShortcut(APP_SHORTCUTS.toggleSidebar, e)) {
    e.preventDefault();
    toggleSidebar();
    return true;
  }

  if (matchesShortcut(APP_SHORTCUTS.increaseLyricsFont, e)) {
    e.preventDefault();
    adjustLyricsFont(1);
    return true;
  }

  if (matchesShortcut(APP_SHORTCUTS.decreaseLyricsFont, e)) {
    e.preventDefault();
    adjustLyricsFont(-1);
    return true;
  }

  if (matchesShortcut(APP_SHORTCUTS.resetLyricsFont, e)) {
    e.preventDefault();
    resetLyricsFont();
    return true;
  }

  const { snapshot, resume, pause, seek, setVolume, positionMs } = player;

  switch (e.code) {
    case "Space": {
      e.preventDefault();
      if (snapshot?.is_playing) {
        pause();
      } else if (snapshot?.song_id) {
        resume();
      }
      return true;
    }
    case "ArrowLeft": {
      e.preventDefault();
      seek(positionMs - 5000);
      return true;
    }
    case "ArrowRight": {
      e.preventDefault();
      seek(positionMs + 5000);
      return true;
    }
    case "ArrowUp": {
      e.preventDefault();
      const volume = snapshot?.volume ?? 1;
      setVolume(Math.min(1, volume + 0.05));
      return true;
    }
    case "ArrowDown": {
      e.preventDefault();
      const volume = snapshot?.volume ?? 1;
      setVolume(Math.max(0, volume - 0.05));
      return true;
    }
    default:
      return false;
  }
}

export function useKeyboardShortcuts(enabled = true): void {
  useEffect(() => {
    if (!enabled) {
      return;
    }

    const handleKeyDown = (e: KeyboardEvent) => {
      handleAppKeyDown(e, {
        toggleSettings: () => useSettingsStore.getState().toggle(),
        toggleSidebar: () => useLayoutStore.getState().toggleSidebar(),
        adjustLyricsFont: (delta) =>
          void useSettingsStore.getState().adjustLyricsFontStep(delta),
        resetLyricsFont: () =>
          void useSettingsStore.getState().resetLyricsFontStep(),
        player: {
          ...usePlayerStore.getState(),
        },
      });
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [enabled]);
}
