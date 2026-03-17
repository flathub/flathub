import type { ReactNode } from "react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";
import { formatBytes } from "./SettingsOverlay.utils";
import type { ModelVariant } from "@/types/ipc";

interface DangerActionRowProps {
  title: ReactNode;
  description: ReactNode;
  actionLabel: ReactNode;
  actionState?: "idle" | "busy" | "disabled";
  actionTitle?: string;
  onClick: () => void;
}

function DangerActionRow({
  title,
  description,
  actionLabel,
  actionState = "idle",
  actionTitle,
  onClick,
}: DangerActionRowProps) {
  return (
    <div className="flex items-center justify-between gap-4">
      <div>
        <p className="text-[13px] text-white">{title}</p>
        <p className="text-[11px] text-[var(--color-text-dim)]">
          {description}
        </p>
      </div>
      <button
        onClick={onClick}
        disabled={actionState !== "idle"}
        title={actionTitle}
        className="shrink-0 rounded-md border border-red-500/40 bg-red-600/10 px-3 py-1.5 text-[12px] text-red-400 transition-colors hover:bg-red-600/20 hover:text-red-300 disabled:opacity-50"
      >
        {actionLabel}
      </button>
    </div>
  );
}

function DeleteModelAction({ variant }: { variant: ModelVariant }) {
  const { t } = useTranslation();
  const { state, actions } = useSettingsOverlay();
  const status = state.modelStatuses[variant];

  if (!status?.downloaded) {
    return null;
  }

  const isActiveModel = state.modelVariant === variant;

  return (
    <DangerActionRow
      title={
        variant === "htdemucs"
          ? t("settings.dangerZone.deleteModelStandard")
          : t("settings.dangerZone.deleteModelHQ")
      }
      description={`${t("settings.dangerZone.deleteModelDescription")}${
        status.file_size ? ` (${formatBytes(status.file_size)})` : ""
      }`}
      actionLabel={t("settings.dangerZone.deleteModelButton")}
      actionState={isActiveModel ? "disabled" : "idle"}
      actionTitle={isActiveModel ? "Cannot delete the active model" : undefined}
      onClick={() => void actions.deleteModel(variant)}
    />
  );
}

export function SettingsDangerZoneSection() {
  const { t } = useTranslation();
  const { meta, actions } = useSettingsOverlay();

  return (
    <SettingsSectionCard title={t("settings.dangerZone.label")} tone="danger">
      <div className="space-y-4">
        <DangerActionRow
          title={t("settings.dangerZone.deleteStems")}
          description={t("settings.dangerZone.deleteStemsDescription")}
          actionLabel={
            meta.deletingStemsInProgress
              ? t("common.deleting")
              : t("settings.dangerZone.deleteStemsButton")
          }
          actionState={meta.deletingStemsInProgress ? "busy" : "idle"}
          onClick={() => void actions.openDeleteStemsDialog()}
        />

        <DangerActionRow
          title={t("settings.dangerZone.downgradeStems")}
          description={t("settings.dangerZone.downgradeStemsDescription")}
          actionLabel={
            meta.downgradingInProgress
              ? t("common.deleting")
              : t("settings.dangerZone.downgradeStemsButton")
          }
          actionState={meta.downgradingInProgress ? "busy" : "idle"}
          onClick={() => void actions.openDowngradeDialog()}
        />

        <DeleteModelAction variant="htdemucs" />
        <DeleteModelAction variant="htdemucs_ft" />

        <DangerActionRow
          title={t("settings.dangerZone.deleteLyrics")}
          description={t("settings.dangerZone.deleteLyricsDescription")}
          actionLabel={
            meta.deletingLyricsInProgress
              ? t("common.deleting")
              : t("settings.dangerZone.deleteLyricsButton")
          }
          actionState={meta.deletingLyricsInProgress ? "busy" : "idle"}
          onClick={() => actions.openDeleteLyricsDialog()}
        />
      </div>
    </SettingsSectionCard>
  );
}
