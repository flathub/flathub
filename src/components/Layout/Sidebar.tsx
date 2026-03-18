import { useState } from "react";
import { useTranslation } from "react-i18next";
import { Folder, CheckCircle2, UploadCloud, Layers } from "lucide-react";
import { ConfirmationDialog } from "@/components/Settings/ConfirmationDialog";
import { SearchBox } from "@/components/Library/SearchBox";
import { SongList } from "@/components/Library/SongList";
import { ImportButton } from "@/components/Library/ImportButton";
import { useLibraryStore } from "@/stores/library-store";
import { useSettingsStore } from "@/stores/settings-store";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";

export function Sidebar() {
  const { t } = useTranslation();
  const songs = useLibraryStore((s) => s.songs);
  const filter = useLibraryStore((s) => s.filter);
  const setFilter = useLibraryStore((s) => s.setFilter);
  const separationStatuses = useLibraryStore((s) => s.separationStatuses);
  const batchSeparation = useLibraryStore((s) => s.batchSeparation);

  const hideBatchSeparate = useSettingsStore((s) => s.hideBatchSeparate);
  const stemMode = useSettingsStore((s) => s.stemMode);
  const [showUpgradeConfirm, setShowUpgradeConfirm] = useState(false);
  const separableSongs = songs.filter((song) => song.media_g_container == null);

  const separatedCount = songs.filter(
    (s) => separationStatuses[s.hash]?.state === "completed",
  ).length;

  // Check if all songs are separated in the current stem mode.
  // Treat "running" as separated — a song being re-separated was previously completed.
  const allSeparated =
    separableSongs.length > 0 &&
    separableSongs.every((s) => {
      const status = separationStatuses[s.hash];
      return status?.state === "completed" || status?.state === "running";
    });

  const allMatchCurrentMode =
    allSeparated &&
    separableSongs.every((s) => {
      const status = separationStatuses[s.hash];
      if (!status) return false;
      if (status.state === "running") return true; // being processed, count as matching
      if (status.state !== "completed") return false;
      if (stemMode === "four_stem") return !!status.drums_path;
      return true; // any completed is fine for two_stem
    });

  const needsUpgrade =
    allSeparated && !allMatchCurrentMode && stemMode === "four_stem";

  const shouldHideButton =
    hideBatchSeparate ||
    separableSongs.length === 0 ||
    (allSeparated && allMatchCurrentMode);

  const handleSeparateAll = () => {
    api.batchSeparate([]).catch(notifyError);
  };

  const isBatchRunning =
    batchSeparation != null &&
    batchSeparation.completed + batchSeparation.failed < batchSeparation.total;

  return (
    <div className="flex h-full w-[260px] shrink-0 flex-col border-r border-[color-mix(in_srgb,var(--color-border)_86%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_94%,transparent)] shadow-[1px_0_0_rgba(255,255,255,0.02)] backdrop-blur-xl">
      <div className="shrink-0 px-3 pb-3 pt-3">
        <SearchBox />
      </div>

      {/* Filter tabs */}
      <div className="shrink-0 space-y-0.5 px-2">
        <div className="px-2 pb-1 text-[11px] font-semibold tracking-wide text-[var(--color-text-dim)]">
          {t("sidebar.library")}
        </div>
        <button
          onClick={() => setFilter("all")}
          className={`motion-surface flex w-full items-center justify-between rounded-md px-2 py-1.5 ${
            filter === "all"
              ? "bg-[color-mix(in_srgb,var(--color-hover)_88%,transparent)] text-white shadow-[0_10px_26px_rgba(0,0,0,0.14)]"
              : "text-[var(--color-text)] hover:bg-[color-mix(in_srgb,var(--color-hover)_82%,transparent)]"
          }`}
        >
          <span className="flex items-center gap-2">
            <Folder
              size={14}
              className="text-[var(--color-accent)]"
              fill="currentColor"
              fillOpacity={0.2}
            />
            <span>{t("sidebar.allTracks")}</span>
          </span>
          <span className="text-[11px] text-[var(--color-text-dim)]">
            {songs.length}
          </span>
        </button>
        <button
          onClick={() => setFilter("separated")}
          className={`motion-surface flex w-full items-center justify-between rounded-md px-2 py-1.5 ${
            filter === "separated"
              ? "bg-[color-mix(in_srgb,var(--color-hover)_88%,transparent)] text-white shadow-[0_10px_26px_rgba(0,0,0,0.14)]"
              : "text-[var(--color-text)] hover:bg-[color-mix(in_srgb,var(--color-hover)_82%,transparent)]"
          }`}
        >
          <span className="flex items-center gap-2">
            <CheckCircle2 size={14} className="text-[var(--color-text-dim)]" />
            <span>{t("sidebar.separated")}</span>
          </span>
          <span className="text-[11px] text-[var(--color-text-dim)]">
            {separatedCount}
          </span>
        </button>
      </div>

      {/* Song list */}
      <div className="mt-4 flex flex-1 flex-col overflow-hidden px-2">
        <div className="flex items-center justify-between px-2 pb-1 text-[11px] font-semibold tracking-wide text-[var(--color-text-dim)]">
          <span>{t("sidebar.localMusic")}</span>
          <ImportButton>
            <UploadCloud
              size={12}
              className="motion-icon-button rounded p-1 hover:bg-white/4 hover:text-white"
            />
          </ImportButton>
        </div>
        <SongList />
      </div>

      {/* Batch separation controls */}
      {!(shouldHideButton && !isBatchRunning && batchSeparation == null) && (
        <div className="shrink-0 border-t border-[var(--color-border)] px-3 py-3">
          {isBatchRunning ? (
            <div className="text-center text-[11px] text-[var(--color-text-dim)]">
              {t("sidebar.separating", {
                current: Math.min(
                  batchSeparation.completed + 1,
                  batchSeparation.total,
                ),
                total: batchSeparation.total,
              })}
            </div>
          ) : batchSeparation != null ? (
            // Completed/cancelled state (shown briefly before clearing)
            <div className="text-center text-[11px] text-[var(--color-text-dim)]">
              {t("sidebar.separationComplete", {
                done: batchSeparation.completed,
              })}
              {batchSeparation.skipped > 0 &&
                `, ${t("sidebar.skipped", { count: batchSeparation.skipped })}`}
              {batchSeparation.failed > 0 &&
                `, ${t("sidebar.failed", { count: batchSeparation.failed })}`}
            </div>
          ) : needsUpgrade ? (
            <button
              onClick={() => setShowUpgradeConfirm(true)}
              className="motion-surface flex w-full items-center justify-center gap-2 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] hover:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_82%,transparent)] hover:text-white"
            >
              <Layers size={12} />
              {t("sidebar.upgradeAll")}
            </button>
          ) : (
            <button
              onClick={handleSeparateAll}
              disabled={separableSongs.length === 0}
              className="motion-surface flex w-full items-center justify-center gap-2 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] hover:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_82%,transparent)] hover:text-white disabled:opacity-40"
            >
              <Layers size={12} />
              {t("sidebar.separateAll")}
              <span className="text-[10px] text-[var(--color-text-dimmer)]">
                (
                {stemMode === "four_stem"
                  ? t("sidebar.fourStem")
                  : t("sidebar.twoStem")}
                )
              </span>
            </button>
          )}
        </div>
      )}

      {showUpgradeConfirm && (
        <ConfirmationDialog
          title={t("sidebar.confirmUpgrade.title")}
          message={t("sidebar.confirmUpgrade.message")}
          confirmLabel={t("sidebar.confirmUpgrade.confirm")}
          onConfirm={() => {
            setShowUpgradeConfirm(false);
            api.batchSeparate([]).catch(notifyError);
          }}
          onCancel={() => setShowUpgradeConfirm(false)}
        />
      )}
    </div>
  );
}
