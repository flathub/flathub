import { useBootstrapStore } from "@/stores/bootstrap-store";
import type {
  SettingsActionContext,
  SettingsOverlayActions,
} from "./settings-overlay.types";

export function createModelSettingsActions(
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
      try {
        await dependencies.api.deleteModel(variant);
        await refreshModelStatuses();
        void useBootstrapStore.getState().loadStatus();
      } catch (error) {
        dependencies.notifyError(error);
      }
    },
  };
}
