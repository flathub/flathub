import type { ReactNode } from "react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";
import { formatBytes } from "./SettingsOverlay.utils";
import type { ModelVariant } from "@/types/ipc";

interface ModelVariantOptionProps {
  selected: boolean;
  disabled: boolean;
  title: ReactNode;
  description: ReactNode;
  status: ReactNode;
  onClick: () => void;
}

function ModelVariantOption({
  selected,
  disabled,
  title,
  description,
  status,
  onClick,
}: ModelVariantOptionProps) {
  return (
    <button
      onClick={onClick}
      disabled={disabled}
      className={`flex-1 rounded-md border px-3 py-2 text-[13px] transition-colors ${
        selected
          ? "border-[var(--color-accent)] bg-[var(--color-accent)]/20 text-white"
          : "border-[var(--color-border-light)] bg-[var(--color-surface)] text-[var(--color-text)] hover:bg-[var(--color-hover)] hover:text-white"
      } disabled:opacity-50`}
    >
      <div className="font-medium">{title}</div>
      <div className="mt-0.5 text-[11px] opacity-70">{description}</div>
      <div className="mt-1 text-[10px] opacity-50">{status}</div>
    </button>
  );
}

export function SettingsModelVariantSection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();

  const modelStatusLabel = (variant: ModelVariant) => {
    if (state.downloadingModel === variant) {
      return t("settings.modelVariant.downloading");
    }

    const status = state.modelStatuses[variant];

    if (status?.downloaded) {
      return `${t("settings.modelVariant.downloaded")}${
        status.file_size ? ` (${formatBytes(status.file_size)})` : ""
      }`;
    }

    return t("settings.modelVariant.notDownloaded");
  };

  const controlsDisabled =
    meta.isInitializing || state.downloadingModel !== null;

  return (
    <SettingsSectionCard
      title={t("settings.modelVariant.label")}
      description={t("settings.modelVariant.description")}
    >
      <div className="flex gap-2">
        <ModelVariantOption
          selected={state.modelVariant === "htdemucs"}
          disabled={controlsDisabled}
          title={t("settings.modelVariant.htdemucs")}
          description={t("settings.modelVariant.htdemucsDescription")}
          status={modelStatusLabel("htdemucs")}
          onClick={() => void actions.selectModelVariant("htdemucs")}
        />
        <ModelVariantOption
          selected={state.modelVariant === "htdemucs_ft"}
          disabled={controlsDisabled}
          title={t("settings.modelVariant.htdemucsFt")}
          description={t("settings.modelVariant.htdemucsFtDescription")}
          status={modelStatusLabel("htdemucs_ft")}
          onClick={() => void actions.selectModelVariant("htdemucs_ft")}
        />
      </div>
    </SettingsSectionCard>
  );
}
