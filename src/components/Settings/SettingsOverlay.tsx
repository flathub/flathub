import { useState, useEffect } from "react";
import { useTranslation } from "react-i18next";
import { open } from "@tauri-apps/plugin-dialog";
import { FolderOpen, Plus } from "lucide-react";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import { SUPPORTED_LANGUAGES } from "@/lib/i18n";
import { ConfirmationDialog } from "./ConfirmationDialog";
import { useLibraryStore } from "@/stores/library-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import type { StemMode } from "@/types/ipc";

function formatBytes(bytes: number): string {
  if (bytes === 0) return "0 B";
  const units = ["B", "KB", "MB", "GB"];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  const value = bytes / Math.pow(1024, i);
  return `${value.toFixed(i > 0 ? 1 : 0)} ${units[i]}`;
}

export function SettingsOverlay() {
  const { t, i18n } = useTranslation();
  const [libraryPath, setLibraryPath] = useState<string | null>(null);
  const [libraryError, setLibraryError] = useState<string | null>(null);
  const [stemMode, setStemMode] = useState<StemMode>("two_stem");
  const [language, setLanguageState] = useState<string>("en");

  // Danger zone dialogs
  const [showDeleteStemsConfirm, setShowDeleteStemsConfirm] = useState(false);
  const [stemsSize, setStemsSize] = useState<number | null>(null);
  const [deletingStemsInProgress, setDeletingStemsInProgress] = useState(false);
  const [showDeleteLyricsConfirm, setShowDeleteLyricsConfirm] = useState(false);
  const [deletingLyricsInProgress, setDeletingLyricsInProgress] = useState(false);

  useEffect(() => {
    api.getLibraryPath().then(setLibraryPath).catch((e) => notifyError(e));
    api
      .getSettings()
      .then((settings) => {
        setStemMode(settings.stem_mode);
        setLanguageState(settings.language ?? "en");
      })
      .catch((e) => notifyError(e));
  }, []);

  const handleLanguageChange = async (lang: string) => {
    setLanguageState(lang);
    i18n.changeLanguage(lang);
    try {
      await api.setLanguage(lang);
    } catch (e) {
      notifyError(e);
    }
  };

  const handleCreateLibrary = async () => {
    const selected = await open({
      directory: true,
      title: t("setup.dialogTitleCreate"),
    });
    if (!selected) return;

    const libraryDir = `${selected}/OpenKara`;
    setLibraryError(null);
    try {
      await api.createLibrary(libraryDir);
      setLibraryPath(libraryDir);
    } catch (err: unknown) {
      setLibraryError(err instanceof Error ? err.message : String(err));
    }
  };

  const handleOpenLibrary = async () => {
    const selected = await open({
      directory: true,
      title: t("setup.dialogTitleOpen"),
    });
    if (!selected) return;

    setLibraryError(null);
    try {
      await api.openLibrary(selected);
      setLibraryPath(selected);
    } catch (err: unknown) {
      setLibraryError(err instanceof Error ? err.message : String(err));
    }
  };

  const handleStemModeChange = async (mode: StemMode) => {
    try {
      const settings = await api.setStemMode(mode);
      setStemMode(settings.stem_mode);
    } catch (e) {
      notifyError(e);
    }
  };

  const handleDeleteStemsClick = async () => {
    try {
      const size = await api.estimateStemsSize();
      setStemsSize(size);
    } catch {
      setStemsSize(null);
    }
    setShowDeleteStemsConfirm(true);
  };

  const handleDeleteStemsConfirm = async () => {
    setDeletingStemsInProgress(true);
    try {
      await api.deleteAllStems();
      // Clear in-memory separation statuses
      useLibraryStore.getState().clearAllSeparationStatuses();
    } catch (e) {
      notifyError(e);
    } finally {
      setDeletingStemsInProgress(false);
      setShowDeleteStemsConfirm(false);
    }
  };

  const handleDeleteLyricsConfirm = async () => {
    setDeletingLyricsInProgress(true);
    try {
      await api.deleteAllCachedLyrics();
      useLyricsStore.getState().clear();
    } catch (e) {
      notifyError(e);
    } finally {
      setDeletingLyricsInProgress(false);
      setShowDeleteLyricsConfirm(false);
    }
  };

  return (
    <div className="flex flex-1 flex-col overflow-y-auto p-10">
      <div className="mx-auto w-full max-w-xl space-y-6">
        <h2 className="text-lg font-semibold text-white">{t("settings.title")}</h2>

        {/* Library Section */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            {t("settings.library.label")}
          </label>
          {libraryPath ? (
            <div className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2">
              <p
                className="truncate text-[13px] text-white"
                title={libraryPath}
              >
                {libraryPath}
              </p>
            </div>
          ) : (
            <p className="text-[13px] text-[var(--color-text-dim)]">
              {t("settings.library.noLibrary")}
            </p>
          )}
          <div className="flex gap-2">
            <button
              onClick={handleCreateLibrary}
              className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
            >
              <Plus size={12} /> {t("settings.library.newLibrary")}
            </button>
            <button
              onClick={handleOpenLibrary}
              className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
            >
              <FolderOpen size={12} /> {t("settings.library.openLibrary")}
            </button>
          </div>
          {libraryError && (
            <p className="text-[12px] text-red-400">{libraryError}</p>
          )}
        </div>

        {/* Stem Mode */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            {t("settings.stemMode.label")}
          </label>
          <p className="text-[12px] text-[var(--color-text-dim)]">
            {t("settings.stemMode.description")}
          </p>
          <div className="flex gap-2">
            <button
              onClick={() => handleStemModeChange("two_stem")}
              className={`flex-1 rounded-md border px-3 py-2 text-[13px] transition-colors ${
                stemMode === "two_stem"
                  ? "border-[var(--color-accent)] bg-[var(--color-accent)]/20 text-white"
                  : "border-[var(--color-border-light)] bg-[var(--color-surface)] text-[var(--color-text)] hover:bg-[var(--color-hover)] hover:text-white"
              }`}
            >
              <div className="font-medium">{t("settings.stemMode.twoStem")}</div>
              <div className="mt-0.5 text-[11px] opacity-70">
                {t("settings.stemMode.twoStemDescription")}
              </div>
            </button>
            <button
              onClick={() => handleStemModeChange("four_stem")}
              className={`flex-1 rounded-md border px-3 py-2 text-[13px] transition-colors ${
                stemMode === "four_stem"
                  ? "border-[var(--color-accent)] bg-[var(--color-accent)]/20 text-white"
                  : "border-[var(--color-border-light)] bg-[var(--color-surface)] text-[var(--color-text)] hover:bg-[var(--color-hover)] hover:text-white"
              }`}
            >
              <div className="font-medium">{t("settings.stemMode.fourStem")}</div>
              <div className="mt-0.5 text-[11px] opacity-70">
                {t("settings.stemMode.fourStemDescription")}
              </div>
            </button>
          </div>
        </div>

        {/* Output Device */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            {t("settings.outputDevice.label")}
          </label>
          <select className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-2 py-1.5 text-[13px] text-white focus:border-[var(--color-accent)] focus:outline-none">
            <option>{t("settings.outputDevice.systemDefault")}</option>
          </select>
        </div>

        {/* Language */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            {t("settings.language.label")}
          </label>
          <select
            value={language}
            onChange={(e) => handleLanguageChange(e.target.value)}
            className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-2 py-1.5 text-[13px] text-white focus:border-[var(--color-accent)] focus:outline-none"
          >
            {SUPPORTED_LANGUAGES.map((lang) => (
              <option key={lang.code} value={lang.code}>
                {lang.name}
              </option>
            ))}
          </select>
        </div>

        {/* Danger Zone */}
        <div className="space-y-4 rounded-lg border border-red-500/30 bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-red-400">
            {t("settings.dangerZone.label")}
          </label>

          {/* Delete All Stems */}
          <div className="flex items-center justify-between gap-4">
            <div>
              <p className="text-[13px] text-white">
                {t("settings.dangerZone.deleteStems")}
              </p>
              <p className="text-[11px] text-[var(--color-text-dim)]">
                {t("settings.dangerZone.deleteStemsDescription")}
              </p>
            </div>
            <button
              onClick={handleDeleteStemsClick}
              disabled={deletingStemsInProgress}
              className="shrink-0 rounded-md border border-red-500/40 bg-red-600/10 px-3 py-1.5 text-[12px] text-red-400 transition-colors hover:bg-red-600/20 hover:text-red-300 disabled:opacity-50"
            >
              {deletingStemsInProgress ? t("common.deleting") : t("settings.dangerZone.deleteStemsButton")}
            </button>
          </div>

          {/* Delete All Lyrics */}
          <div className="flex items-center justify-between gap-4">
            <div>
              <p className="text-[13px] text-white">
                {t("settings.dangerZone.deleteLyrics")}
              </p>
              <p className="text-[11px] text-[var(--color-text-dim)]">
                {t("settings.dangerZone.deleteLyricsDescription")}
              </p>
            </div>
            <button
              onClick={() => setShowDeleteLyricsConfirm(true)}
              disabled={deletingLyricsInProgress}
              className="shrink-0 rounded-md border border-red-500/40 bg-red-600/10 px-3 py-1.5 text-[12px] text-red-400 transition-colors hover:bg-red-600/20 hover:text-red-300 disabled:opacity-50"
            >
              {deletingLyricsInProgress ? t("common.deleting") : t("settings.dangerZone.deleteLyricsButton")}
            </button>
          </div>
        </div>
      </div>

      {/* Confirmation Dialogs */}
      {showDeleteStemsConfirm && (
        <ConfirmationDialog
          title={t("settings.confirmDeleteStems.title")}
          message={t("settings.confirmDeleteStems.message")}
          detail={
            stemsSize != null && stemsSize > 0
              ? t("settings.confirmDeleteStems.detail", { size: formatBytes(stemsSize) })
              : undefined
          }
          confirmLabel={t("settings.confirmDeleteStems.confirm")}
          onConfirm={handleDeleteStemsConfirm}
          onCancel={() => setShowDeleteStemsConfirm(false)}
        />
      )}

      {showDeleteLyricsConfirm && (
        <ConfirmationDialog
          title={t("settings.confirmDeleteLyrics.title")}
          message={t("settings.confirmDeleteLyrics.message")}
          confirmLabel={t("settings.confirmDeleteLyrics.confirm")}
          onConfirm={handleDeleteLyricsConfirm}
          onCancel={() => setShowDeleteLyricsConfirm(false)}
        />
      )}
    </div>
  );
}
