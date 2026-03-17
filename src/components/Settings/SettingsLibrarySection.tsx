import { FolderOpen, Plus } from "lucide-react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";

export function SettingsLibrarySection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();

  return (
    <SettingsSectionCard title={t("settings.library.label")}>
      {state.libraryPath ? (
        <div className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2">
          <p
            className="truncate text-[13px] text-white"
            title={state.libraryPath}
          >
            {state.libraryPath}
          </p>
        </div>
      ) : (
        <p className="text-[13px] text-[var(--color-text-dim)]">
          {t("settings.library.noLibrary")}
        </p>
      )}

      <div className="flex gap-2">
        <button
          onClick={() =>
            void actions.createLibrary(t("setup.dialogTitleCreate"))
          }
          disabled={meta.isInitializing}
          className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white disabled:opacity-50"
        >
          <Plus size={12} /> {t("settings.library.newLibrary")}
        </button>
        <button
          onClick={() => void actions.openLibrary(t("setup.dialogTitleOpen"))}
          disabled={meta.isInitializing}
          className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white disabled:opacity-50"
        >
          <FolderOpen size={12} /> {t("settings.library.openLibrary")}
        </button>
      </div>

      {state.libraryError && (
        <p className="text-[12px] text-red-400">{state.libraryError}</p>
      )}
    </SettingsSectionCard>
  );
}
