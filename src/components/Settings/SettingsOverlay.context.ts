import { createContext, use } from "react";
import {
  createInitialSettingsOverlaySnapshot,
  type SettingsOverlayActions,
  type SettingsOverlayMeta,
  type SettingsOverlayState,
} from "./SettingsOverlay.state";

export interface SettingsOverlayContextValue {
  state: SettingsOverlayState;
  meta: SettingsOverlayMeta;
  actions: SettingsOverlayActions;
}

const defaultSnapshot = createInitialSettingsOverlaySnapshot();

export const SettingsOverlayContext =
  createContext<SettingsOverlayContextValue | null>(null);

export function useSettingsOverlay(): SettingsOverlayContextValue {
  const context = use(SettingsOverlayContext);

  if (!context) {
    throw new Error(
      "SettingsOverlay components must be used within the provider.",
    );
  }

  return context;
}

export function createSettingsOverlayTestContextValue(
  snapshot: {
    state?: Partial<SettingsOverlayState>;
    meta?: Partial<SettingsOverlayMeta>;
  } = {},
  actions: Partial<SettingsOverlayActions> = {},
): SettingsOverlayContextValue {
  return {
    state: {
      ...defaultSnapshot.state,
      ...snapshot.state,
    },
    meta: {
      ...defaultSnapshot.meta,
      ...snapshot.meta,
    },
    actions: {
      initialize: async () => {},
      createLibrary: async () => {},
      openLibrary: async () => {},
      switchLibrary: async () => {},
      refreshRemoteRepository: async () => {},
      renameLibrary: async () => {},
      removeLibrary: async () => {},
      deleteLibrary: async () => {},
      setLanguage: async () => {},
      restartApp: async () => {},
      setStemMode: async () => {},
      setExecutionProvider: async () => {},
      selectModelVariant: async () => {},
      confirmFtModel: async () => {},
      deleteModel: async () => {},
      toggleHideBatchSeparate: async () => {},
      toggleCoverArtBackdrop: async () => {},
      openDeleteStemsDialog: async () => {},
      confirmDeleteStems: async () => {},
      openDowngradeDialog: async () => {},
      confirmDowngrade: async () => {},
      openDeleteLyricsDialog: () => {},
      confirmDeleteLyrics: async () => {},
      closeDialog: () => {},
      refreshModelStatuses: async () => {},
      ...actions,
    },
  };
}
