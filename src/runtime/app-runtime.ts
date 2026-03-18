import { useEffect } from "react";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import { useSettingsStore } from "@/stores/settings-store";
import {
  useEventListeners,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import { useCdgSync } from "@/hooks/use-cdg-sync";
import { useKeyboardShortcuts } from "@/hooks/use-keyboard-shortcuts";
import { useFileDrop } from "@/hooks/use-file-drop";
import { notifyError } from "@/lib/errors";
import * as api from "@/lib/tauri";
import i18next, { detectSystemLanguage } from "@/lib/i18n";
import { useAppMenuRuntime } from "./menu-runtime";
import { loadStartupSettings } from "./settings-runtime";

export function useAppStartupRuntime(
  libraryReady: boolean | null,
  setLibraryReady: (ready: boolean) => void,
) {
  const loadLibrary = useLibraryStore((s) => s.loadLibrary);
  const loadBootstrapStatus = useBootstrapStore((s) => s.loadStatus);
  const loadPlayerState = usePlayerStore((s) => s.loadState);
  const hydrateAppSettings = useSettingsStore((s) => s.hydrateAppSettings);

  useEffect(() => {
    void loadStartupSettings({
      getSettings: api.getSettings,
      hydrateAppSettings,
      changeLanguage: i18next.changeLanguage,
      detectFallbackLanguage: detectSystemLanguage,
    }).catch(() => {
      // Language persistence should not block first render if settings are
      // temporarily unavailable; later settings actions will rehydrate state.
    });
  }, [hydrateAppSettings]);

  useEffect(() => {
    api
      .getLibraryPath()
      .then((path) => setLibraryReady(path !== null))
      .catch((error) => {
        notifyError(error);
        setLibraryReady(false);
      });
  }, [setLibraryReady]);

  useEffect(() => {
    if (!libraryReady) {
      return;
    }

    void loadLibrary();
    void loadBootstrapStatus();
    void loadPlayerState();
  }, [libraryReady, loadBootstrapStatus, loadLibrary, loadPlayerState]);
}

export function useMainWindowRuntimeWhen(enabled: boolean) {
  // Keep main-window side effects behind the library-ready gate so first-run
  // setup does not accidentally enable file-drop imports or playback listeners
  // before a library exists.
  useEventListeners(enabled);
  useLyricsAutoFetch(enabled);
  useLyricsSync(enabled);
  useCdgSync(enabled);
  useKeyboardShortcuts(enabled);
  useFileDrop(enabled);
  useAppMenuRuntime(enabled);
}
