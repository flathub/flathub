import { open } from "@tauri-apps/plugin-dialog";
import * as api from "@/lib/tauri";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { usePlayerStore } from "@/stores/player-store";
import { useQueueStore } from "@/stores/queue-store";
import { useSettingsStore } from "@/stores/settings-store";
import type {
  ExecutionProvider,
  LibraryRegistrySnapshot,
  ModelVariant,
  RegisteredLibrary,
  StemMode,
} from "@/types/ipc";

export type DangerDialog =
  | "delete_stems"
  | "downgrade_stems"
  | "delete_lyrics"
  | "ft_warning"
  | null;

export interface ModelStatusView {
  downloaded: boolean;
  legacy_install_present: boolean;
  file_size: number | null;
}

export interface SettingsOverlayState {
  libraryPath: string | null;
  libraryError: string | null;
  libraryRegistry: LibraryRegistrySnapshot | null;
  libraries: RegisteredLibrary[];
  activeLibraryId: string | null;
  stemMode: StemMode;
  modelVariant: ModelVariant;
  modelStatuses: Partial<Record<ModelVariant, ModelStatusView>>;
  downloadingModel: ModelVariant | null;
  language: string;
  hideBatchSeparate: boolean;
  coverArtBackdrop: boolean;
  executionProvider: ExecutionProvider;
  availableExecutionProviders: ExecutionProvider[];
}

export interface SettingsOverlayMeta {
  isInitializing: boolean;
  dangerDialog: DangerDialog;
  stemsSize: number | null;
  downgradeSavings: number | null;
  deletingStemsInProgress: boolean;
  deletingLyricsInProgress: boolean;
  downgradingInProgress: boolean;
}

export interface SettingsOverlaySnapshot {
  state: SettingsOverlayState;
  meta: SettingsOverlayMeta;
}

export interface SettingsOverlayActions {
  initialize: () => Promise<void>;
  createLibrary: (dialogTitle: string) => Promise<void>;
  openLibrary: (dialogTitle: string) => Promise<void>;
  switchLibrary: (libraryId: string) => Promise<void>;
  refreshRemoteRepository: (libraryId: string) => Promise<void>;
  renameLibrary: (libraryId: string) => Promise<void>;
  removeLibrary: (libraryId: string) => Promise<void>;
  deleteLibrary: (libraryId: string) => Promise<void>;
  setLanguage: (language: string) => Promise<void>;
  restartApp: () => Promise<void>;
  setStemMode: (mode: StemMode) => Promise<void>;
  setExecutionProvider: (provider: ExecutionProvider) => Promise<void>;
  selectModelVariant: (variant: ModelVariant) => Promise<void>;
  confirmFtModel: () => Promise<void>;
  deleteModel: (variant: ModelVariant) => Promise<void>;
  toggleHideBatchSeparate: (value: boolean) => Promise<void>;
  toggleCoverArtBackdrop: (value: boolean) => Promise<void>;
  openDeleteStemsDialog: () => Promise<void>;
  confirmDeleteStems: () => Promise<void>;
  openDowngradeDialog: () => Promise<void>;
  confirmDowngrade: () => Promise<void>;
  openDeleteLyricsDialog: () => void;
  confirmDeleteLyrics: () => Promise<void>;
  closeDialog: () => void;
  refreshModelStatuses: () => Promise<void>;
}

export interface SettingsOverlayControllerDependencies {
  api: Pick<
    typeof api,
    | "createLibrary"
    | "createLocalLibrary"
    | "deleteAllCachedLyrics"
    | "deleteAllStems"
    | "deleteModel"
    | "downloadModel"
    | "downgradeAllToTwoStem"
    | "estimateDowngradeSavings"
    | "estimateStemsSize"
    | "getAllSeparationStatuses"
    | "getLibraryPath"
    | "getLibraryRegistry"
    | "getSettings"
    | "getModelStatus"
    | "openLibrary"
    | "registerLocalLibrary"
    | "renameLibrary"
    | "removeLibrary"
    | "deleteLibrary"
    | "mirrorLocalLibraryToRemote"
    | "reauthorizeRemoteLibrary"
    | "restartApp"
    | "switchLibrary"
    | "refreshRemoteRepository"
    | "setExecutionProvider"
    | "setHideBatchSeparate"
    | "setCoverArtBackdrop"
    | "setLanguage"
    | "setModelVariant"
    | "setStemMode"
  >;
  notifyError: (error: unknown) => void;
  openDirectory: typeof open;
  changeLanguage: (language: string) => void | Promise<unknown>;
  libraryStore: Pick<
    ReturnType<typeof useLibraryStore.getState>,
    | "clearAllSeparationStatuses"
    | "clearAllUploadStatuses"
    | "clearSelection"
    | "loadLibrary"
    | "updateSeparationStatus"
  >;
  queueStore: Pick<ReturnType<typeof useQueueStore.getState>, "clearQueue">;
  playerStore: Pick<ReturnType<typeof usePlayerStore.getState>, "loadState">;
  lyricsStore: Pick<ReturnType<typeof useLyricsStore.getState>, "clear">;
  settingsStore: Pick<
    ReturnType<typeof useSettingsStore.getState>,
    "getAppSettingsSnapshot" | "hydrateAppSettings" | "patchAppSettings"
  >;
}

export interface SettingsOverlayStateControls {
  getSnapshot: () => SettingsOverlaySnapshot;
  setSnapshot: (
    updater: (previous: SettingsOverlaySnapshot) => SettingsOverlaySnapshot,
  ) => void;
}

export type PatchState = (patch: Partial<SettingsOverlayState>) => void;
export type PatchMeta = (patch: Partial<SettingsOverlayMeta>) => void;

export interface SettingsActionContext {
  dependencies: SettingsOverlayControllerDependencies;
  controls: SettingsOverlayStateControls;
  patchState: PatchState;
  patchMeta: PatchMeta;
  refreshLibraryRegistry: () => Promise<void>;
  refreshModelStatuses: () => Promise<void>;
  applyModelVariant: (variant: ModelVariant) => Promise<void>;
  selectSingleDirectory: (dialogTitle: string) => Promise<string | null>;
  closeDialog: () => void;
}
