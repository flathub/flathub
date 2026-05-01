import { getErrorMessage } from "@/lib/errors";
import { useLibraryStore } from "@/stores/library-store";
import type { LibraryRegistrySnapshot, RegisteredLibrary } from "@/types/ipc";
import type {
  SettingsActionContext,
  SettingsOverlayActions,
} from "./settings-overlay.types";

async function applyLibrarySwitchSideEffects(
  context: SettingsActionContext,
  libraryId: string,
  registry: LibraryRegistrySnapshot,
  options: { syncRemote?: boolean } = {},
) {
  const target = registry.libraries.find((library) => library.id === libraryId);
  if (target?.kind === "remote" && options.syncRemote !== false) {
    await context.dependencies.api.syncActiveRemoteLibrary();
  }

  context.dependencies.libraryStore.clearAllSeparationStatuses();
  context.dependencies.libraryStore.clearAllUploadStatuses();
  context.dependencies.libraryStore.clearSelection();
  context.dependencies.queueStore.clearQueue();
  context.dependencies.lyricsStore.clear();
  await context.dependencies.playerStore.loadState();
  await context.dependencies.libraryStore.loadLibrary();
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

function remoteProviderDisplayName(library: RegisteredLibrary): string {
  if (library.kind !== "remote") {
    return "the selected storage provider";
  }

  return library.provider === "google_drive"
    ? "Google Drive"
    : library.provider === "dropbox"
      ? "Dropbox"
      : "WebDAV";
}

export function createLibrarySettingsActions(
  context: SettingsActionContext,
): Pick<
  SettingsOverlayActions,
  | "createLibrary"
  | "openLibrary"
  | "switchLibrary"
  | "forceSyncRemoteLibrary"
  | "renameLibrary"
  | "removeLibrary"
  | "deleteLibrary"
  | "setLanguage"
  | "restartApp"
  | "setStemMode"
  | "setExecutionProvider"
  | "toggleHideBatchSeparate"
> {
  const {
    dependencies,
    controls,
    patchState,
    refreshLibraryRegistry,
    refreshModelStatuses,
    selectSingleDirectory,
  } = context;

  const refreshLibraryStateAfterMutation = async (
    registry?: LibraryRegistrySnapshot,
  ) => {
    const nextRegistry =
      registry ?? (await dependencies.api.getLibraryRegistry());
    if (nextRegistry.active_library_id) {
      await dependencies.libraryStore.loadLibrary();
    } else {
      useLibraryStore.setState({ songs: [], searchQuery: "" });
      dependencies.libraryStore.clearAllSeparationStatuses();
      dependencies.libraryStore.clearAllUploadStatuses();
      dependencies.libraryStore.clearSelection();
      dependencies.queueStore.clearQueue();
      dependencies.lyricsStore.clear();
      await dependencies.playerStore.loadState();
    }
    await refreshLibraryRegistry();
    await refreshModelStatuses();
    return nextRegistry;
  };

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

    forceSyncRemoteLibrary: async (libraryId) => {
      patchState({ libraryError: null });

      const current = controls.getSnapshot().state;
      const target = current.libraries.find(
        (library) => library.id === libraryId,
      );
      if (target?.kind !== "remote") {
        return;
      }

      try {
        if (current.activeLibraryId !== libraryId) {
          const registry = await dependencies.api.switchLibrary(libraryId);
          await applyLibrarySwitchSideEffects(context, libraryId, registry);
          return;
        }

        await dependencies.api.syncActiveRemoteLibrary();
        const registry = await dependencies.api.getLibraryRegistry();
        await applyLibrarySwitchSideEffects(context, libraryId, registry, {
          syncRemote: false,
        });
      } catch (error: unknown) {
        patchState({
          libraryError: getErrorMessage(error),
        });
      }
    },

    renameLibrary: async (libraryId) => {
      patchState({ libraryError: null });

      const currentLibrary = controls
        .getSnapshot()
        .state.libraries.find((library) => library.id === libraryId);
      const currentName = currentLibrary?.display_name ?? "";
      const nextName = window.prompt(
        currentLibrary?.kind === "remote"
          ? "Rename the remote repository"
          : "Rename the local library",
        currentName,
      );
      if (nextName == null) {
        return;
      }

      const trimmedName = nextName.trim();
      if (!trimmedName || trimmedName === currentName) {
        return;
      }

      try {
        const registry = await dependencies.api.renameLibrary(
          libraryId,
          trimmedName,
        );
        await refreshLibraryStateAfterMutation(registry);
      } catch (error: unknown) {
        patchState({
          libraryError: getErrorMessage(error),
        });
      }
    },

    removeLibrary: async (libraryId) => {
      patchState({ libraryError: null });

      const currentLibrary = controls
        .getSnapshot()
        .state.libraries.find((library) => library.id === libraryId);

      if (
        !window.confirm(
          currentLibrary?.kind === "remote"
            ? `Disconnect "${currentLibrary.display_name}" from OpenKara? The remote repository contents will stay in ${remoteProviderDisplayName(currentLibrary)}.`
            : "Disconnect this library from OpenKara? The library data will stay on disk.",
        )
      ) {
        return;
      }

      try {
        const registry = await dependencies.api.removeLibrary(libraryId);
        await refreshLibraryStateAfterMutation(registry);
      } catch (error: unknown) {
        patchState({
          libraryError: getErrorMessage(error),
        });
      }
    },

    deleteLibrary: async (libraryId) => {
      patchState({ libraryError: null });

      const currentLibrary = controls
        .getSnapshot()
        .state.libraries.find((library) => library.id === libraryId);
      const isRemoteRepository = currentLibrary?.kind === "remote";
      const displayName = currentLibrary?.display_name ?? "this library";

      if (
        !window.confirm(
          isRemoteRepository
            ? `Delete "${displayName}"? This will delete the remote repository contents from ${remoteProviderDisplayName(currentLibrary)} at ${describeLibrary(currentLibrary)} and remove the local working copy.`
            : `Delete "${displayName}" from OpenKara? This removes the local library data from disk.`,
        )
      ) {
        return;
      }

      const typedConfirmation = window.prompt(
        `Type "${displayName}" to confirm permanent deletion.`,
        "",
      );
      if (typedConfirmation !== displayName) {
        return;
      }

      try {
        const registry = await dependencies.api.deleteLibrary(libraryId);
        await refreshLibraryStateAfterMutation(registry);
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
