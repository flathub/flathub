import { open } from "@tauri-apps/plugin-dialog";
import { useEffect, useMemo, useRef, useState, type ReactNode } from "react";
import { useTranslation } from "react-i18next";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useSettingsStore } from "@/stores/settings-store";
import {
  SettingsOverlayContext,
  type SettingsOverlayContextValue,
} from "./SettingsOverlay.context";
import {
  createInitialSettingsOverlaySnapshot,
  createSettingsOverlayActions,
  type SettingsOverlayControllerDependencies,
  type SettingsOverlaySnapshot,
} from "./SettingsOverlay.state";

export interface SettingsOverlayProviderProps {
  children: ReactNode;
  dependencies?: Partial<SettingsOverlayControllerDependencies>;
  initialSnapshot?: SettingsOverlaySnapshot;
  skipInitialize?: boolean;
}

export function SettingsOverlayProvider({
  children,
  dependencies,
  initialSnapshot,
  skipInitialize = false,
}: SettingsOverlayProviderProps) {
  const { i18n } = useTranslation();
  const [snapshot, setSnapshot] = useState<SettingsOverlaySnapshot>(
    initialSnapshot ?? createInitialSettingsOverlaySnapshot(),
  );
  const snapshotRef = useRef(snapshot);
  snapshotRef.current = snapshot;

  const stateControls = useMemo(
    () => ({
      getSnapshot: () => snapshotRef.current,
      setSnapshot: (
        updater: (previous: SettingsOverlaySnapshot) => SettingsOverlaySnapshot,
      ) => {
        setSnapshot((previous) => {
          const next = updater(previous);
          snapshotRef.current = next;
          return next;
        });
      },
    }),
    [],
  );

  const defaultDependencies = useMemo<SettingsOverlayControllerDependencies>(
    () => ({
      api,
      notifyError,
      openDirectory: open,
      changeLanguage: (language: string) => i18n.changeLanguage(language),
      libraryStore: {
        clearAllSeparationStatuses:
          useLibraryStore.getState().clearAllSeparationStatuses,
        updateSeparationStatus:
          useLibraryStore.getState().updateSeparationStatus,
      },
      lyricsStore: {
        clear: useLyricsStore.getState().clear,
      },
      settingsStore: {
        getAppSettingsSnapshot:
          useSettingsStore.getState().getAppSettingsSnapshot,
        hydrateAppSettings: useSettingsStore.getState().hydrateAppSettings,
        patchAppSettings: useSettingsStore.getState().patchAppSettings,
      },
    }),
    [i18n],
  );

  const resolvedDependencies = useMemo(
    () => ({
      ...defaultDependencies,
      ...dependencies,
    }),
    [defaultDependencies, dependencies],
  );

  const actions = useMemo(
    () => createSettingsOverlayActions(resolvedDependencies, stateControls),
    [resolvedDependencies, stateControls],
  );

  useEffect(() => {
    if (skipInitialize) {
      return;
    }

    void actions.initialize();
  }, [actions, skipInitialize]);

  const value = useMemo<SettingsOverlayContextValue>(
    () => ({
      state: snapshot.state,
      meta: snapshot.meta,
      actions,
    }),
    [actions, snapshot.meta, snapshot.state],
  );

  return (
    <SettingsOverlayContext value={value}>{children}</SettingsOverlayContext>
  );
}
