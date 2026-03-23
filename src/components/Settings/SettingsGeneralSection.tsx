import { useTranslation } from "react-i18next";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { SUPPORTED_LANGUAGES } from "@/lib/i18n";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";

export function SettingsGeneralSection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();
  const macPlatform = getShortcutPlatform() === "mac";
  const shellRestartPending =
    !meta.isInitializing && meta.appliedMacOsShellMode !== state.macosShellMode;

  return (
    <SettingsSectionCard title={t("settings.language.label")}>
      <div className="space-y-4">
        <div className="space-y-2">
          <label className="flex cursor-pointer items-center gap-3">
            <input
              type="checkbox"
              checked={state.hideBatchSeparate}
              onChange={(event) =>
                void actions.toggleHideBatchSeparate(event.target.checked)
              }
              disabled={meta.isInitializing}
              className="h-4 w-4 rounded border-[var(--color-border-light)] bg-[var(--color-surface)] accent-[var(--color-accent)]"
            />
            <span className="text-[13px] text-white">
              {t("settings.hideBatchSeparate.hide")}
            </span>
          </label>
          <p className="text-[11px] text-[var(--color-text-dim)]">
            {t("settings.hideBatchSeparate.description")}
          </p>
        </div>

        <div className="space-y-2 border-t border-[var(--color-border)] pt-4">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            {t("settings.outputDevice.label")}
          </label>
          <select className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-2 py-1.5 text-[13px] text-white focus:border-[var(--color-accent)] focus:outline-none">
            <option>{t("settings.outputDevice.systemDefault")}</option>
          </select>
        </div>

        <div className="space-y-2 border-t border-[var(--color-border)] pt-4">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            {t("settings.language.label")}
          </label>
          <select
            value={state.language}
            onChange={(event) => void actions.setLanguage(event.target.value)}
            disabled={meta.isInitializing}
            className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-2 py-1.5 text-[13px] text-white focus:border-[var(--color-accent)] focus:outline-none disabled:opacity-50"
          >
            {SUPPORTED_LANGUAGES.map((language) => (
              <option key={language.code} value={language.code}>
                {language.name}
              </option>
            ))}
          </select>
        </div>

        {macPlatform ? (
          <div className="space-y-2 border-t border-[var(--color-border)] pt-4">
            <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
              {t("settings.macosShellMode.label")}
            </label>
            <select
              value={state.macosShellMode}
              onChange={(event) =>
                void actions.setMacOsShellMode(
                  event.target.value as "stable" | "native",
                )
              }
              disabled={meta.isInitializing}
              className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-2 py-1.5 text-[13px] text-white focus:border-[var(--color-accent)] focus:outline-none disabled:opacity-50"
            >
              <option value="stable">
                {t("settings.macosShellMode.stable")}
              </option>
              <option value="native">
                {t("settings.macosShellMode.native")}
              </option>
            </select>
            <p className="text-[11px] text-[var(--color-text-dim)]">
              {t("settings.macosShellMode.restartRequired")}
            </p>
            {shellRestartPending ? (
              <button
                type="button"
                onClick={() => void actions.restartApp()}
                className="rounded-md border border-[var(--color-accent)] bg-[var(--color-accent)] px-3 py-1.5 text-[12px] font-medium text-black transition hover:brightness-110"
              >
                {t("settings.macosShellMode.restartButton")}
              </button>
            ) : null}
          </div>
        ) : null}
      </div>
    </SettingsSectionCard>
  );
}
