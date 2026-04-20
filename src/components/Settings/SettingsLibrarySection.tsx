import { FolderOpen, Plus, CheckCircle2, Library, Globe } from "lucide-react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { useSettingsOverlay } from "./SettingsOverlay.context";

export function SettingsLibrarySection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();
  const hasLibraries = state.libraries.length > 0;

  return (
    <SettingsSectionCard title={t("settings.library.label")}>
      {!hasLibraries ? (
        <p className="text-[13px] text-[var(--color-text-dim)]">
          {t("settings.library.noLibrary")}
        </p>
      ) : (
        <div className="space-y-2">
          {state.libraries.map((library) => {
            const isActive = library.id === state.activeLibraryId;
            const isRemote = library.kind === "remote";
            return (
              <button
                key={library.id}
                onClick={() => void actions.switchLibrary(library.id)}
                disabled={meta.isInitializing || isActive}
                className={`flex w-full items-center gap-3 rounded-md border px-3 py-2 text-left transition-colors ${
                  isActive
                    ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                    : "border-[var(--color-border-light)] bg-[var(--color-surface)] hover:bg-[var(--color-hover)]"
                }`}
              >
                {isRemote ? (
                  <Globe
                    size={12}
                    className="shrink-0 text-[var(--color-accent)]"
                  />
                ) : (
                  <Library
                    size={12}
                    className="shrink-0 text-[var(--color-text-dim)]"
                  />
                )}
                <div className="min-w-0 flex-1">
                  <p className="truncate text-[13px] text-white">
                    {library.display_name}
                  </p>
                  <p className="truncate text-[11px] text-[var(--color-text-dim)]">
                    {library.root_path}
                  </p>
                </div>
                {isActive ? (
                  <CheckCircle2
                    size={14}
                    className="shrink-0 text-[var(--color-accent)]"
                  />
                ) : null}
              </button>
            );
          })}
        </div>
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
