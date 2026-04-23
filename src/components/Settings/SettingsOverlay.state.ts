import { useBootstrapStore } from "@/stores/bootstrap-store";
import { useSettingsStore } from "@/stores/settings-store";
import type { LibraryRegistrySnapshot, ModelVariant } from "@/types/ipc";
import {
  createLibrarySettingsActions,
  describeLibrary,
} from "./settings-overlay.library-actions";
import { createMaintenanceSettingsActions } from "./settings-overlay.maintenance-actions";
import { createModelSettingsActions } from "./settings-overlay.model-actions";
import type {
  SettingsActionContext,
  SettingsOverlayActions,
  SettingsOverlayControllerDependencies,
  SettingsOverlayMeta,
  SettingsOverlaySnapshot,
  SettingsOverlayState,
  SettingsOverlayStateControls,
} from "./settings-overlay.types";

export type {
  DangerDialog,
  ModelStatusView,
  PatchMeta,
  PatchState,
  SettingsActionContext,
  SettingsOverlayActions,
  SettingsOverlayControllerDependencies,
  SettingsOverlayMeta,
  SettingsOverlaySnapshot,
  SettingsOverlayState,
  SettingsOverlayStateControls,
} from "./settings-overlay.types";

export function createInitialSettingsOverlaySnapshot(
  initialSettings = useSettingsStore.getState().getAppSettingsSnapshot(),
): SettingsOverlaySnapshot {
  return {
    state: {
      libraryPath: null,
      libraryError: null,
      libraryRegistry: null,
      libraries: [],
      activeLibraryId: null,
      stemMode: initialSettings.stemMode,
      modelVariant: initialSettings.modelVariant,
      modelStatuses: {},
      downloadingModel: null,
      language: initialSettings.language ?? "en",
      hideBatchSeparate: initialSettings.hideBatchSeparate,
      executionProvider: initialSettings.executionProvider,
      availableExecutionProviders: initialSettings.availableExecutionProviders,
    },
    meta: {
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

  const applyRegistrySnapshot = (registry: LibraryRegistrySnapshot) => {
    const activeLibrary = registry.libraries.find(
      (library) => library.id === registry.active_library_id,
    );

    patchState({
      libraryRegistry: registry,
      libraries: registry.libraries,
      activeLibraryId: registry.active_library_id,
      libraryPath: activeLibrary ? describeLibrary(activeLibrary) : null,
      libraryError: null,
    });
  };

  const refreshLibraryRegistry = async () => {
    try {
      const registry = await dependencies.api.getLibraryRegistry();
      applyRegistrySnapshot(registry);
    } catch (error) {
      dependencies.notifyError(error);
    }
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
            legacy_install_present: standard.legacy_install_present,
            file_size: standard.file_size,
          },
          htdemucs_ft: {
            downloaded: hq.downloaded,
            legacy_install_present: hq.legacy_install_present,
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
        void useBootstrapStore.getState().loadStatus();
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
    refreshLibraryRegistry,
    refreshModelStatuses,
    applyModelVariant,
    selectSingleDirectory,
    closeDialog,
  };

  return {
    initialize: async () => {
      patchMeta({ isInitializing: true });

      const [registryResult, settingsResult, windowShellResult] =
        await Promise.allSettled([
          dependencies.api.getLibraryRegistry(),
          dependencies.api.getSettings(),
          dependencies.api.getWindowShellState(),
        ]);

      if (registryResult.status === "fulfilled") {
        applyRegistrySnapshot(registryResult.value);
      } else {
        dependencies.notifyError(registryResult.reason);
      }

      if (windowShellResult.status === "fulfilled") {
        void windowShellResult.value;
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
          executionProvider: settingsResult.value.execution_provider,
          availableExecutionProviders:
            settingsResult.value.available_execution_providers,
        });
      } else {
        dependencies.notifyError(settingsResult.reason);
      }

      patchMeta({ isInitializing: false });

      void refreshModelStatuses();
    },

    refreshModelStatuses,
    ...createLibrarySettingsActions(actionContext),
    ...createModelSettingsActions(actionContext),
    ...createMaintenanceSettingsActions(actionContext),
    closeDialog,
  };
}
