import { useState, useEffect } from "react";
import { open } from "@tauri-apps/plugin-dialog";
import { FolderOpen, Plus } from "lucide-react";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
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
  const [libraryPath, setLibraryPath] = useState<string | null>(null);
  const [libraryError, setLibraryError] = useState<string | null>(null);
  const [stemMode, setStemMode] = useState<StemMode>("two_stem");

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
      .then((settings) => setStemMode(settings.stem_mode))
      .catch((e) => notifyError(e));
  }, []);

  const handleCreateLibrary = async () => {
    const selected = await open({
      directory: true,
      title: "Choose a location for your Karaoke Library",
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
      title: "Open an existing Karaoke Library",
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
    } catch (e) {
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
        <h2 className="text-lg font-semibold text-white">Preferences</h2>

        {/* Library Section */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            Karaoke Library
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
              No library configured
            </p>
          )}
          <div className="flex gap-2">
            <button
              onClick={handleCreateLibrary}
              className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
            >
              <Plus size={12} /> New Library
            </button>
            <button
              onClick={handleOpenLibrary}
              className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
            >
              <FolderOpen size={12} /> Open Library
            </button>
          </div>
          {libraryError && (
            <p className="text-[12px] text-red-400">{libraryError}</p>
          )}
        </div>

        {/* Stem Mode */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            Stem Separation Mode
          </label>
          <p className="text-[12px] text-[var(--color-text-dim)]">
            Controls how new songs are separated. Existing separations are not
            affected.
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
              <div className="font-medium">2-Stem</div>
              <div className="mt-0.5 text-[11px] opacity-70">
                Vocals + Accompaniment
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
              <div className="font-medium">4-Stem</div>
              <div className="mt-0.5 text-[11px] opacity-70">
                Vocals + Drums + Bass + Other
              </div>
            </button>
          </div>
        </div>

        {/* Output Device */}
        <div className="space-y-3 rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-[var(--color-text-dim)]">
            Output Device
          </label>
          <select className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-2 py-1.5 text-[13px] text-white focus:border-[var(--color-accent)] focus:outline-none">
            <option>System Default</option>
          </select>
        </div>

        {/* Danger Zone */}
        <div className="space-y-4 rounded-lg border border-red-500/30 bg-[var(--color-sidebar)] p-5">
          <label className="text-[12px] font-medium uppercase text-red-400">
            Danger Zone
          </label>

          {/* Delete All Stems */}
          <div className="flex items-center justify-between gap-4">
            <div>
              <p className="text-[13px] text-white">
                Delete All Separated Stems
              </p>
              <p className="text-[11px] text-[var(--color-text-dim)]">
                Remove all cached stem files to free disk space
              </p>
            </div>
            <button
              onClick={handleDeleteStemsClick}
              disabled={deletingStemsInProgress}
              className="shrink-0 rounded-md border border-red-500/40 bg-red-600/10 px-3 py-1.5 text-[12px] text-red-400 transition-colors hover:bg-red-600/20 hover:text-red-300 disabled:opacity-50"
            >
              {deletingStemsInProgress ? "Deleting..." : "Delete Stems"}
            </button>
          </div>

          {/* Delete All Lyrics */}
          <div className="flex items-center justify-between gap-4">
            <div>
              <p className="text-[13px] text-white">
                Delete All Cached Lyrics
              </p>
              <p className="text-[11px] text-[var(--color-text-dim)]">
                Remove all fetched and cached lyrics. Lyrics will be re-fetched
                when songs are played.
              </p>
            </div>
            <button
              onClick={() => setShowDeleteLyricsConfirm(true)}
              disabled={deletingLyricsInProgress}
              className="shrink-0 rounded-md border border-red-500/40 bg-red-600/10 px-3 py-1.5 text-[12px] text-red-400 transition-colors hover:bg-red-600/20 hover:text-red-300 disabled:opacity-50"
            >
              {deletingLyricsInProgress ? "Deleting..." : "Delete Lyrics"}
            </button>
          </div>
        </div>
      </div>

      {/* Confirmation Dialogs */}
      {showDeleteStemsConfirm && (
        <ConfirmationDialog
          title="Delete All Separated Stems"
          message="This will permanently delete all cached stem files. Songs will need to be re-separated to use karaoke mode."
          detail={
            stemsSize != null && stemsSize > 0
              ? `This will free approximately ${formatBytes(stemsSize)}.`
              : undefined
          }
          confirmLabel="Delete All Stems"
          onConfirm={handleDeleteStemsConfirm}
          onCancel={() => setShowDeleteStemsConfirm(false)}
        />
      )}

      {showDeleteLyricsConfirm && (
        <ConfirmationDialog
          title="Delete All Cached Lyrics"
          message="This will permanently delete all cached lyrics. Lyrics will be automatically re-fetched when songs are played."
          confirmLabel="Delete All Lyrics"
          onConfirm={handleDeleteLyricsConfirm}
          onCancel={() => setShowDeleteLyricsConfirm(false)}
        />
      )}
    </div>
  );
}
