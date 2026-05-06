import { X } from "lucide-react";
import { useTranslation } from "react-i18next";
import { SettingsDangerZoneSection } from "./SettingsDangerZoneSection";
import { SettingsDialogHost } from "./SettingsDialogHost";
import { SettingsExecutionProviderSection } from "./SettingsExecutionProviderSection";
import { SettingsGeneralSection } from "./SettingsGeneralSection";
import { SettingsLibrarySection } from "./SettingsLibrarySection";
import { SettingsModelVariantSection } from "./SettingsModelVariantSection";
import { SettingsOverlayProvider } from "./SettingsOverlay.controller";
import { SettingsStemModeSection } from "./SettingsStemModeSection";
import { useSettingsStore } from "@/stores/settings-store";

export function SettingsOverlay() {
  const { t } = useTranslation();
  const closeSettings = useSettingsStore((s) => s.close);

  return (
    <div className="pointer-events-auto absolute inset-0 z-20 flex flex-1 flex-col overflow-y-auto bg-[var(--color-surface-muted)]/98 p-10 backdrop-blur-sm">
      <div className="mx-auto w-full max-w-xl space-y-6">
        <div className="flex items-start justify-between gap-4">
          <h2 className="text-lg font-semibold text-white">
            {t("settings.title")}
          </h2>
          <button
            type="button"
            onClick={closeSettings}
            aria-label={t("common.close")}
            className="motion-icon-button rounded-xl p-2 text-[var(--color-text-dim)] hover:bg-white/6 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
          >
            <X size={16} />
          </button>
        </div>
        <SettingsOverlayProvider>
          <SettingsLibrarySection />
          <SettingsStemModeSection />
          <SettingsModelVariantSection />
          <SettingsExecutionProviderSection />
          <SettingsGeneralSection />
          <SettingsDangerZoneSection />
          <SettingsDialogHost />
        </SettingsOverlayProvider>
      </div>
    </div>
  );
}
