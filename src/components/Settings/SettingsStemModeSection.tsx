import type { ReactNode } from "react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";
import type { StemMode } from "@/types/ipc";

interface StemModeOptionProps {
  selected: boolean;
  disabled: boolean;
  title: ReactNode;
  description: ReactNode;
  onClick: () => void;
}

function StemModeOption({
  selected,
  disabled,
  title,
  description,
  onClick,
}: StemModeOptionProps) {
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
    </button>
  );
}

export function SettingsStemModeSection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();

  const selectMode = (mode: StemMode) => {
    void actions.setStemMode(mode);
  };

  return (
    <SettingsSectionCard
      title={t("settings.stemMode.label")}
      description={t("settings.stemMode.description")}
    >
      <div className="flex gap-2">
        <StemModeOption
          selected={state.stemMode === "two_stem"}
          disabled={meta.isInitializing}
          title={t("settings.stemMode.twoStem")}
          description={t("settings.stemMode.twoStemDescription")}
          onClick={() => selectMode("two_stem")}
        />
        <StemModeOption
          selected={state.stemMode === "four_stem"}
          disabled={meta.isInitializing}
          title={t("settings.stemMode.fourStem")}
          description={t("settings.stemMode.fourStemDescription")}
          onClick={() => selectMode("four_stem")}
        />
      </div>
    </SettingsSectionCard>
  );
}
