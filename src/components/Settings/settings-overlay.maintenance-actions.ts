import type {
  SettingsActionContext,
  SettingsOverlayActions,
} from "./settings-overlay.types";

export function createMaintenanceSettingsActions(
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
