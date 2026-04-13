import type { ReactNode } from "react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";
import type { ExecutionProvider } from "@/types/ipc";

interface ExecutionProviderOptionProps {
  selected: boolean;
  disabled: boolean;
  title: ReactNode;
  description: ReactNode;
  onClick: () => void;
}

function ExecutionProviderOption({
  selected,
  disabled,
  title,
  description,
  onClick,
}: ExecutionProviderOptionProps) {
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

function useEpLabels() {
  const { t } = useTranslation();
  return {
    cpu: {
      title: t("settings.executionProvider.cpu"),
      description: t("settings.executionProvider.cpuDescription"),
    },
    coreml: {
      title: t("settings.executionProvider.coreml"),
      description: t("settings.executionProvider.coremlDescription"),
    },
    directml: {
      title: t("settings.executionProvider.directml"),
      description: t("settings.executionProvider.directmlDescription"),
    },
  } satisfies Record<ExecutionProvider, { title: string; description: string }>;
}

export function SettingsExecutionProviderSection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();
  const labels = useEpLabels();

  const selectProvider = (provider: ExecutionProvider) => {
    void actions.setExecutionProvider(provider);
  };

  return (
    <SettingsSectionCard
      title={t("settings.executionProvider.label")}
      description={t("settings.executionProvider.description")}
    >
      <div className="flex gap-2">
        {state.availableExecutionProviders.map((provider) => (
          <ExecutionProviderOption
            key={provider}
            selected={state.executionProvider === provider}
            disabled={meta.isInitializing}
            title={labels[provider].title}
            description={labels[provider].description}
            onClick={() => selectProvider(provider)}
          />
        ))}
      </div>
      <p className="text-[11px] text-[var(--color-text-dim)]">
        {t("settings.executionProvider.note")}
      </p>
    </SettingsSectionCard>
  );
}
