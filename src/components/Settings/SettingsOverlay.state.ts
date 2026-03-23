import { open } from "@tauri-apps/plugin-dialog";
import * as api from "@/lib/tauri";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useSettingsStore } from "@/stores/settings-store";
import type {
  MacOsShellMode,
  ModelVariant,
  StemMode,
  WindowShellStateSnapshot,
} from "@/types/ipc";

export type DangerDialog =
  | "delete_stems"
  | "downgrade_stems"
  | "delete_lyrics"
  | "ft_warning"
  | null;

export interface ModelStatusView {
  downloaded: boolean;
  file_size: number | null;
}

export interface SettingsOverlayState {
  libraryPath: string | null;
  libraryError: string | null;
  stemMode: StemMode;
  modelVariant: ModelVariant;
  modelStatuses: Partial<Record<ModelVariant, ModelStatusView>>;
  downloadingModel: ModelVariant | null;
  language: string;
  hideBatchSeparate: boolean;
  macosShellMode: MacOsShellMode;
}

export interface SettingsOverlayMeta {
  appliedMacOsShellMode: MacOsShellMode;
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
  setLanguage: (language: string) => Promise<void>;
  setMacOsShellMode: (mode: MacOsShellMode) => Promise<void>;
  restartApp: () => Promise<void>;
  setStemMode: (mode: StemMode) => Promise<void>;
  selectModelVariant: (variant: ModelVariant) => Promise<void>;
  confirmFtModel: () => Promise<void>;
  deleteModel: (variant: ModelVariant) => Promise<void>;
  toggleHideBatchSeparate: (value: boolean) => Promise<void>;
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
    | "deleteAllCachedLyrics"
    | "deleteAllStems"
    | "deleteModel"
    | "downloadModel"
    | "downgradeAllToTwoStem"
    | "estimateDowngradeSavings"
    | "estimateStemsSize"
    | "getAllSeparationStatuses"
    | "getLibraryPath"
    | "getSettings"
    | "getWindowShellState"
    | "getModelStatus"
    | "openLibrary"
    | "restartApp"
    | "setHideBatchSeparate"
    | "setLanguage"
    | "setMacOsShellMode"
    | "setModelVariant"
    | "setStemMode"
  >;
  notifyError: (error: unknown) => void;
  openDirectory: typeof open;
  changeLanguage: (language: string) => void | Promise<unknown>;
  libraryStore: Pick<
    ReturnType<typeof useLibraryStore.getState>,
    "clearAllSeparationStatuses" | "updateSeparationStatus"
  >;
  lyricsStore: Pick<ReturnType<typeof useLyricsStore.getState>, "clear">;
  settingsStore: Pick<
    ReturnType<typeof useSettingsStore.getState>,
    "getAppSettingsSnapshot" | "hydrateAppSettings" | "patchAppSettings"
  >;
}

interface SettingsOverlayStateControls {
  getSnapshot: () => SettingsOverlaySnapshot;
  setSnapshot: (
    updater: (previous: SettingsOverlaySnapshot) => SettingsOverlaySnapshot,
  ) => void;
}

type PatchState = (patch: Partial<SettingsOverlayState>) => void;
type PatchMeta = (patch: Partial<SettingsOverlayMeta>) => void;

interface SettingsActionContext {
  dependencies: SettingsOverlayControllerDependencies;
  controls: SettingsOverlayStateControls;
  patchState: PatchState;
  patchMeta: PatchMeta;
  refreshModelStatuses: () => Promise<void>;
  applyModelVariant: (variant: ModelVariant) => Promise<void>;
  selectSingleDirectory: (dialogTitle: string) => Promise<string | null>;
  closeDialog: () => void;
}

export function createInitialSettingsOverlaySnapshot(
  initialSettings = useSettingsStore.getState().getAppSettingsSnapshot(),
): SettingsOverlaySnapshot {
  return {
    state: {
      libraryPath: null,
      libraryError: null,
      stemMode: initialSettings.stemMode,
      modelVariant: initialSettings.modelVariant,
      modelStatuses: {},
      downloadingModel: null,
      language: initialSettings.language ?? "en",
      hideBatchSeparate: initialSettings.hideBatchSeparate,
      macosShellMode: initialSettings.macosShellMode,
    },
    meta: {
      appliedMacOsShellMode: "stable",
      isInitializing: true,
      dangerDialog: null,
      stemsSize: null,
      downgradeSavings: null,
      deletingStemsInProgress: false,
      deletingLyricsInProgress: false,
      downgradingInProgress: false,
    },
  };
}

function appliedMacOsShellModeFromWindowShell(
  shellState: WindowShellStateSnapshot,
): MacOsShellMode {
  return shellState.tier === "mac_native" ? "native" : "stable";
}

export function createSettingsOverlayActions(
  dependencies: SettingsOverlayControllerDependencies,
  controls: SettingsOverlayStateControls,
): SettingsOverlayActions {
  const patchState = (patch: Partial<SettingsOverlayState>) => {
    controls.setSnapshot((previous) => ({
      ...previous,
      state: {
        ...previous.state,
        ...patch,
      },
    }));
  };

  const patchMeta = (patch: Partial<SettingsOverlayMeta>) => {
    controls.setSnapshot((previous) => ({
      ...previous,
      meta: {
        ...previous.meta,
        ...patch,
      },
    }));
  };

  const refreshModelStatuses = async () => {
    try {
      const [standard, hq] = await Promise.all([
        dependencies.api.getModelStatus("htdemucs"),
        dependencies.api.getModelStatus("htdemucs_ft"),
      ]);

      patchState({
        modelStatuses: {
          htdemucs: {
            downloaded: standard.downloaded,
            file_size: standard.file_size,
          },
          htdemucs_ft: {
            downloaded: hq.downloaded,
            file_size: hq.file_size,
          },
        },
      });
    } catch {
      // Model status is display-only and should not block the rest of settings.
    }
  };

  const applyModelVariant = async (variant: ModelVariant) => {
    try {
      const current = controls.getSnapshot();
      const status = current.state.modelStatuses[variant];

      if (!status?.downloaded) {
        patchState({ downloadingModel: variant });
        await dependencies.api.downloadModel(variant);
        await refreshModelStatuses();
        patchState({ downloadingModel: null });
      }

      const settings = await dependencies.api.setModelVariant(variant);
      dependencies.settingsStore.hydrateAppSettings(settings);
      patchState({ modelVariant: settings.model_variant });
    } catch (error) {
      patchState({ downloadingModel: null });
      dependencies.notifyError(error);
    }
  };

  const selectSingleDirectory = async (dialogTitle: string) => {
    const selected = await dependencies.openDirectory({
      directory: true,
      title: dialogTitle,
    });

    if (!selected) {
      return null;
    }

    return typeof selected === "string" ? selected : (selected[0] ?? null);
  };

  const closeDialog = () => {
    patchMeta({ dangerDialog: null });
  };

  const actionContext: SettingsActionContext = {
    dependencies,
    controls,
    patchState,
    patchMeta,
    refreshModelStatuses,
    applyModelVariant,
    selectSingleDirectory,
    closeDialog,
  };

  return {
    initialize: async () => {
      patchMeta({ isInitializing: true });

      const [libraryPathResult, settingsResult, windowShellResult] =
        await Promise.allSettled([
          dependencies.api.getLibraryPath(),
          dependencies.api.getSettings(),
          dependencies.api.getWindowShellState(),
        ]);

      if (libraryPathResult.status === "fulfilled") {
        patchState({ libraryPath: libraryPathResult.value });
      } else {
        dependencies.notifyError(libraryPathResult.reason);
      }

      if (windowShellResult.status === "fulfilled") {
        patchMeta({
          appliedMacOsShellMode: appliedMacOsShellModeFromWindowShell(
            windowShellResult.value,
          ),
        });
      } else {
        dependencies.notifyError(windowShellResult.reason);
      }

      if (settingsResult.status === "fulfilled") {
        dependencies.settingsStore.hydrateAppSettings(settingsResult.value);
        patchState({
          stemMode: settingsResult.value.stem_mode,
          modelVariant: settingsResult.value.model_variant,
          language: settingsResult.value.language ?? "en",
          hideBatchSeparate: settingsResult.value.hide_batch_separate,
          macosShellMode: settingsResult.value.macos_shell_mode,
        });
      } else {
        dependencies.notifyError(settingsResult.reason);
      }

      await refreshModelStatuses();

      patchMeta({ isInitializing: false });
    },

    refreshModelStatuses,
    ...createLibrarySettingsActions(actionContext),
    ...createModelSettingsActions(actionContext),
    ...createMaintenanceSettingsActions(actionContext),
    closeDialog,
  };
}

function createLibrarySettingsActions(
  context: SettingsActionContext,
): Pick<
  SettingsOverlayActions,
  | "createLibrary"
  | "openLibrary"
  | "setLanguage"
  | "setMacOsShellMode"
  | "restartApp"
  | "setStemMode"
  | "toggleHideBatchSeparate"
> {
  const { dependencies, patchState, selectSingleDirectory } = context;

  return {
    createLibrary: async (dialogTitle) => {
      const selectedDirectory = await selectSingleDirectory(dialogTitle);
      if (!selectedDirectory) return;

      const libraryDir = `${selectedDirectory}/OpenKara`;
      patchState({ libraryError: null });

      try {
        await dependencies.api.createLibrary(libraryDir);
        patchState({ libraryPath: libraryDir });
      } catch (error: unknown) {
        patchState({
          libraryError: error instanceof Error ? error.message : String(error),
        });
      }
    },

    openLibrary: async (dialogTitle) => {
      const selectedDirectory = await selectSingleDirectory(dialogTitle);
      if (!selectedDirectory) return;

      patchState({ libraryError: null });

      try {
        await dependencies.api.openLibrary(selectedDirectory);
        patchState({ libraryPath: selectedDirectory });
      } catch (error: unknown) {
        patchState({
          libraryError: error instanceof Error ? error.message : String(error),
        });
      }
    },

    setLanguage: async (language) => {
      patchState({ language });
      dependencies.settingsStore.patchAppSettings({ language });
      await Promise.resolve(dependencies.changeLanguage(language));

      try {
        const settings = await dependencies.api.setLanguage(language);
        dependencies.settingsStore.hydrateAppSettings(settings);
      } catch (error) {
        dependencies.notifyError(error);
      }
    },

    setMacOsShellMode: async (mode: MacOsShellMode) => {
      try {
        const settings = await dependencies.api.setMacOsShellMode(mode);
        dependencies.settingsStore.patchAppSettings({
          macosShellMode: settings.macos_shell_mode,
        });
        dependencies.settingsStore.hydrateAppSettings(settings);
        patchState({ macosShellMode: settings.macos_shell_mode });
      } catch (error) {
        dependencies.notifyError(error);
      }
    },

    restartApp: async () => {
      try {
        await dependencies.api.restartApp();
      } catch (error) {
        dependencies.notifyError(error);
      }
    },

    setStemMode: async (mode) => {
      try {
        const settings = await dependencies.api.setStemMode(mode);
        dependencies.settingsStore.hydrateAppSettings(settings);
        patchState({ stemMode: settings.stem_mode });
      } catch (error) {
        dependencies.notifyError(error);
      }
    },

    toggleHideBatchSeparate: async (value) => {
      patchState({ hideBatchSeparate: value });
      dependencies.settingsStore.patchAppSettings({ hideBatchSeparate: value });

      try {
        const settings = await dependencies.api.setHideBatchSeparate(value);
        dependencies.settingsStore.hydrateAppSettings(settings);
      } catch (error) {
        dependencies.notifyError(error);
      }
    },
  };
}

function createModelSettingsActions(
  context: SettingsActionContext,
): Pick<
  SettingsOverlayActions,
  "selectModelVariant" | "confirmFtModel" | "deleteModel"
> {
  const {
    dependencies,
    controls,
    patchMeta,
    refreshModelStatuses,
    applyModelVariant,
    closeDialog,
  } = context;

  return {
    selectModelVariant: async (variant) => {
      const currentVariant = controls.getSnapshot().state.modelVariant;

      if (variant === "htdemucs_ft" && currentVariant !== "htdemucs_ft") {
        patchMeta({ dangerDialog: "ft_warning" });
        return;
      }

      await applyModelVariant(variant);
    },

    confirmFtModel: async () => {
      closeDialog();
      await applyModelVariant("htdemucs_ft");
    },

    deleteModel: async (variant) => {
      if (variant === controls.getSnapshot().state.modelVariant) {
        return;
      }

      try {
        await dependencies.api.deleteModel(variant);
        await refreshModelStatuses();
      } catch (error) {
        dependencies.notifyError(error);
      }
    },
  };
}

function createMaintenanceSettingsActions(
  context: SettingsActionContext,
): Pick<
  SettingsOverlayActions,
  | "openDeleteStemsDialog"
  | "confirmDeleteStems"
  | "openDowngradeDialog"
  | "confirmDowngrade"
  | "openDeleteLyricsDialog"
  | "confirmDeleteLyrics"
> {
  const { dependencies, patchMeta } = context;

  return {
    openDeleteStemsDialog: async () => {
      try {
        const stemsSize = await dependencies.api.estimateStemsSize();
        patchMeta({ stemsSize, dangerDialog: "delete_stems" });
      } catch {
        patchMeta({ stemsSize: null, dangerDialog: "delete_stems" });
      }
    },

    confirmDeleteStems: async () => {
      patchMeta({ deletingStemsInProgress: true });

      try {
        await dependencies.api.deleteAllStems();
        dependencies.libraryStore.clearAllSeparationStatuses();
      } catch (error) {
        dependencies.notifyError(error);
      } finally {
        patchMeta({
          deletingStemsInProgress: false,
          dangerDialog: null,
        });
      }
    },

    openDowngradeDialog: async () => {
      try {
        const downgradeSavings =
          await dependencies.api.estimateDowngradeSavings();
        patchMeta({ downgradeSavings, dangerDialog: "downgrade_stems" });
      } catch {
        patchMeta({ downgradeSavings: null, dangerDialog: "downgrade_stems" });
      }
    },

    confirmDowngrade: async () => {
      patchMeta({ downgradingInProgress: true });

      try {
        await dependencies.api.downgradeAllToTwoStem();
        const statuses = await dependencies.api.getAllSeparationStatuses();

        dependencies.libraryStore.clearAllSeparationStatuses();
        for (const status of statuses) {
          dependencies.libraryStore.updateSeparationStatus(status);
        }
      } catch (error) {
        dependencies.notifyError(error);
      } finally {
        patchMeta({
          downgradingInProgress: false,
          dangerDialog: null,
        });
      }
    },

    openDeleteLyricsDialog: () => {
      patchMeta({ dangerDialog: "delete_lyrics" });
    },

    confirmDeleteLyrics: async () => {
      patchMeta({ deletingLyricsInProgress: true });

      try {
        await dependencies.api.deleteAllCachedLyrics();
        dependencies.lyricsStore.clear();
      } catch (error) {
        dependencies.notifyError(error);
      } finally {
        patchMeta({
          deletingLyricsInProgress: false,
          dangerDialog: null,
        });
      }
    },
  };
}
