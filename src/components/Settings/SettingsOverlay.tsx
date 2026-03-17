import { useTranslation } from "react-i18next";
import { SettingsDangerZoneSection } from "./SettingsDangerZoneSection";
import { SettingsDialogHost } from "./SettingsDialogHost";
import { SettingsGeneralSection } from "./SettingsGeneralSection";
import { SettingsLibrarySection } from "./SettingsLibrarySection";
import { SettingsModelVariantSection } from "./SettingsModelVariantSection";
import { SettingsOverlayProvider } from "./SettingsOverlay.controller";
import { SettingsStemModeSection } from "./SettingsStemModeSection";

export function SettingsOverlay() {
  const { t } = useTranslation();

  return (
    <SettingsOverlayProvider>
      <div className="flex flex-1 flex-col overflow-y-auto p-10">
        <div className="mx-auto w-full max-w-xl space-y-6">
          <h2 className="text-lg font-semibold text-white">
            {t("settings.title")}
          </h2>
          <SettingsLibrarySection />
          <SettingsStemModeSection />
          <SettingsModelVariantSection />
          <SettingsGeneralSection />
          <SettingsDangerZoneSection />
        </div>

        <SettingsDialogHost />
      </div>
    </SettingsOverlayProvider>
  );
}
