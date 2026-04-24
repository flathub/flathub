import { useState } from "react";
import {
  CheckCircle2,
  FolderOpen,
  Globe,
  Library,
  PencilLine,
  Plus,
  Trash2,
  Unlink2,
} from "lucide-react";
import { useTranslation } from "react-i18next";
import { SettingsSectionCard } from "./SettingsSectionCard";
import { RemoteLibraryWizard } from "./RemoteLibraryWizard";
import { useSettingsOverlay } from "./SettingsOverlay.context";
import type { RegisteredLibrary, RemoteLibraryProvider } from "@/types/ipc";

const remoteProviderLabels: Record<RemoteLibraryProvider, string> = {
  google_drive: "Google Drive",
  dropbox: "Dropbox",
  webdav: "WebDAV",
};

function describeLibrarySubtitle(library: RegisteredLibrary) {
  if (library.kind === "local") {
    return library.root_path;
  }

  return `${remoteProviderLabels[library.provider]} · ${
    library.remote_path_display || library.remote_root_locator
  }`;
}

export function SettingsLibrarySection() {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();
  const hasLibraries = state.libraries.length > 0;
  const [remoteWizardOpen, setRemoteWizardOpen] = useState(false);
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
              <div
                key={library.id}
                className={`flex w-full items-center gap-3 rounded-md border px-3 py-2 text-left transition-colors ${
                  isActive
                    ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                    : "border-[var(--color-border-light)] bg-[var(--color-surface)] hover:bg-[var(--color-hover)]"
                }`}
              >
                <button
                  onClick={() => void actions.switchLibrary(library.id)}
                  disabled={meta.isInitializing || isActive}
                  className="flex min-w-0 flex-1 items-center gap-3 text-left"
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
                      {describeLibrarySubtitle(library)}
                    </p>
                  </div>
                  {isActive ? (
                    <CheckCircle2
                      size={14}
                      className="shrink-0 text-[var(--color-accent)]"
                    />
                  ) : null}
                </button>
                <div className="flex shrink-0 items-center gap-1">
                  <button
                    type="button"
                    onClick={() => void actions.renameLibrary(library.id)}
                    disabled={meta.isInitializing}
                    title={t("settings.library.renameLibrary", {
                      defaultValue: "Rename library",
                    })}
                    className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] p-1.5 text-[var(--color-text-dim)] transition-colors hover:bg-[var(--color-hover)] hover:text-white disabled:opacity-50"
                  >
                    <PencilLine size={12} />
                  </button>
                  <button
                    type="button"
                    onClick={() => void actions.removeLibrary(library.id)}
                    disabled={meta.isInitializing}
                    title={t("settings.library.removeLibrary", {
                      defaultValue: "Disconnect library",
                    })}
                    className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] p-1.5 text-[var(--color-text-dim)] transition-colors hover:bg-[var(--color-hover)] hover:text-white disabled:opacity-50"
                  >
                    <Unlink2 size={12} />
                  </button>
                  <button
                    type="button"
                    onClick={() => void actions.deleteLibrary(library.id)}
                    disabled={meta.isInitializing}
                    title={t("settings.library.deleteLibrary", {
                      defaultValue: "Delete library",
                    })}
                    className="rounded-md border border-red-500/40 bg-red-600/10 p-1.5 text-red-400 transition-colors hover:bg-red-600/20 hover:text-red-300 disabled:opacity-50"
                  >
                    <Trash2 size={12} />
                  </button>
                </div>
              </div>
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
        <button
          onClick={() => setRemoteWizardOpen(true)}
          disabled={meta.isInitializing}
          className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white disabled:opacity-50"
        >
          <Globe size={12} />{" "}
          {t("settings.library.addRemoteLibrary", {
            defaultValue: "Add Remote Library",
          })}
        </button>
      </div>

      {state.libraryError && (
        <p className="text-[12px] text-red-400">{state.libraryError}</p>
      )}

      {remoteWizardOpen && (
        <RemoteLibraryWizard onClose={() => setRemoteWizardOpen(false)} />
      )}
    </SettingsSectionCard>
  );
}
