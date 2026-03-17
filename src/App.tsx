import { useEffect, useCallback, useState } from "react";
import { AppLayout } from "@/components/Layout/AppLayout";
import { LibrarySetup } from "@/components/Settings/LibrarySetup";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import {
  useEventListeners,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import { useKeyboardShortcuts } from "@/hooks/use-keyboard-shortcuts";
import { useFileDrop } from "@/hooks/use-file-drop";
import { notifyError } from "@/lib/errors";
import * as api from "@/lib/tauri";
import i18next, { detectSystemLanguage } from "@/lib/i18n";

function App() {
  const [libraryReady, setLibraryReady] = useState<boolean | null>(null);
  const loadLibrary = useLibraryStore((s) => s.loadLibrary);
  const loadBootstrapStatus = useBootstrapStore((s) => s.loadStatus);

  // Check if a library is configured on mount
  useEffect(() => {
    api
      .getLibraryPath()
      .then((path) => setLibraryReady(path !== null))
      .catch((e) => {
        notifyError(e);
        setLibraryReady(false);
      });
  }, []);

  // Load initial data once library is ready
  useEffect(() => {
    if (libraryReady) {
      loadLibrary();
      loadBootstrapStatus();
      usePlayerStore.getState().loadState();
      api
        .getSettings()
        .then((settings) => {
          const lang = settings.language ?? detectSystemLanguage();
          i18next.changeLanguage(lang);
        })
        .catch(() => {});
    }
  }, [libraryReady, loadLibrary, loadBootstrapStatus]);

  // Set up all Tauri event listeners
  useEventListeners();

  // Auto-fetch lyrics when song changes
  useLyricsAutoFetch();

  // Activate lyrics sync rAF loop
  useLyricsSync();

  // Global keyboard shortcuts
  useKeyboardShortcuts();

  // File drop import
  useFileDrop();

  const handleLibrarySetupComplete = useCallback(() => {
    setLibraryReady(true);
  }, []);

  // Show nothing while checking library state
  if (libraryReady === null) {
    return null;
  }

  // Show setup wizard if no library is configured
  if (!libraryReady) {
    return <LibrarySetup onComplete={handleLibrarySetupComplete} />;
  }

  return <AppLayout />;
}
export default App;
