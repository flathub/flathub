import { getErrorMessage } from "@/lib/errors";
import type { LibraryRegistrySnapshot, RegisteredLibrary } from "@/types/ipc";
import type {
  SettingsActionContext,
  SettingsOverlayActions,
} from "./settings-overlay.types";

async function applyLibrarySwitchSideEffects(
  context: SettingsActionContext,
  libraryId: string,
  registry: LibraryRegistrySnapshot,
) {
  const target = registry.libraries.find((library) => library.id === libraryId);
  if (target?.kind === "remote") {
    await context.dependencies.api.syncActiveRemoteLibrary();
  }

  context.dependencies.libraryStore.clearAllSeparationStatuses();
  context.dependencies.libraryStore.clearAllUploadStatuses();
  context.dependencies.libraryStore.clearSelection();
  context.dependencies.queueStore.clearQueue();
  context.dependencies.lyricsStore.clear();
  await context.dependencies.playerStore.loadState();
  await context.refreshLibraryRegistry();
  await context.refreshModelStatuses();
}

export function describeLibrary(library: RegisteredLibrary): string {
  if (library.kind === "local") {
    return library.root_path;
  }

  return `${library.display_name} · ${
    library.remote_path_display || library.remote_root_locator
  }`;
}

export function createLibrarySettingsActions(
  context: SettingsActionContext,
): Pick<
  SettingsOverlayActions,
  | "createLibrary"
  | "openLibrary"
  | "switchLibrary"
  | "setLanguage"
  | "restartApp"
  | "setStemMode"
  | "setExecutionProvider"
  | "toggleHideBatchSeparate"
> {
  const {
    dependencies,
    patchState,
    refreshLibraryRegistry,
    selectSingleDirectory,
  } = context;

  return {
    createLibrary: async (dialogTitle) => {
      const selectedDirectory = await selectSingleDirectory(dialogTitle);
      if (!selectedDirectory) return;

      const libraryDir = `${selectedDirectory}/OpenKara`;
      patchState({ libraryError: null });

      try {
        await dependencies.api.createLocalLibrary(libraryDir);
        await refreshLibraryRegistry();
      } catch (error: unknown) {
        patchState({
          libraryError: getErrorMessage(error),
        });
      }
    },

    openLibrary: async (dialogTitle) => {
      const selectedDirectory = await selectSingleDirectory(dialogTitle);
      if (!selectedDirectory) return;

      patchState({ libraryError: null });

      try {
        await dependencies.api.registerLocalLibrary(selectedDirectory);
        await refreshLibraryRegistry();
      } catch (error: unknown) {
        patchState({
          libraryError: getErrorMessage(error),
        });
      }
    },

    switchLibrary: async (libraryId) => {
      patchState({ libraryError: null });

      try {
        const registry = await dependencies.api.switchLibrary(libraryId);
        await applyLibrarySwitchSideEffects(context, libraryId, registry);
      } catch (error: unknown) {
        patchState({
          libraryError: getErrorMessage(error),
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

    setExecutionProvider: async (provider) => {
      try {
        const settings = await dependencies.api.setExecutionProvider(provider);
        dependencies.settingsStore.hydrateAppSettings(settings);
        patchState({ executionProvider: settings.execution_provider });
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
