import { useState, useEffect } from "react";
import { Folder, CheckCircle2, UploadCloud, Layers, X } from "lucide-react";
import { SearchBox } from "@/components/Library/SearchBox";
import { SongList } from "@/components/Library/SongList";
import { ImportButton } from "@/components/Library/ImportButton";
import { useLibraryStore } from "@/stores/library-store";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import type { StemMode } from "@/types/ipc";

export function Sidebar() {
  const songs = useLibraryStore((s) => s.songs);
  const filter = useLibraryStore((s) => s.filter);
  const setFilter = useLibraryStore((s) => s.setFilter);
  const separationStatuses = useLibraryStore((s) => s.separationStatuses);
  const batchSeparation = useLibraryStore((s) => s.batchSeparation);

  const [stemMode, setStemMode] = useState<StemMode>("two_stem");

  useEffect(() => {
    api
      .getSettings()
      .then((settings) => setStemMode(settings.stem_mode))
      .catch(() => {});
  }, []);

  const separatedCount = songs.filter(
    (s) => separationStatuses[s.hash]?.state === "completed",
  ).length;

  const handleSeparateAll = () => {
    api.batchSeparate([]).catch(notifyError);
  };

  const handleCancelBatch = () => {
    api.cancelBatchSeparation().catch(notifyError);
  };

  const isBatchRunning = batchSeparation != null && batchSeparation.completed + batchSeparation.failed < batchSeparation.total;

  return (
    <div className="flex h-full w-[260px] shrink-0 flex-col border-r border-[var(--color-border)] bg-[var(--color-sidebar)]">
      {/* Spacer for native macOS traffic light buttons */}
      <div
        className="h-12 shrink-0"
        data-tauri-drag-region
      />

      <div className="shrink-0 px-3 pb-3">
        <SearchBox />
      </div>

      {/* Filter tabs */}
      <div className="shrink-0 space-y-0.5 px-2">
        <div className="px-2 pb-1 text-[11px] font-semibold tracking-wide text-[var(--color-text-dim)]">
          LIBRARY
        </div>
        <button
          onClick={() => setFilter("all")}
          className={`flex w-full items-center justify-between rounded-md px-2 py-1.5 ${
            filter === "all"
              ? "bg-[var(--color-hover)] text-white"
              : "text-[var(--color-text)] hover:bg-[var(--color-hover)]"
          }`}
        >
          <span className="flex items-center gap-2">
            <Folder
              size={14}
              className="text-[var(--color-accent)]"
              fill="currentColor"
              fillOpacity={0.2}
            />
            <span>All Tracks</span>
          </span>
          <span className="text-[11px] text-[var(--color-text-dim)]">
            {songs.length}
          </span>
        </button>
        <button
          onClick={() => setFilter("separated")}
          className={`flex w-full items-center justify-between rounded-md px-2 py-1.5 ${
            filter === "separated"
              ? "bg-[var(--color-hover)] text-white"
              : "text-[var(--color-text)] hover:bg-[var(--color-hover)]"
          }`}
        >
          <span className="flex items-center gap-2">
            <CheckCircle2 size={14} className="text-[var(--color-text-dim)]" />
            <span>Separated</span>
          </span>
          <span className="text-[11px] text-[var(--color-text-dim)]">
            {separatedCount}
          </span>
        </button>
      </div>

      {/* Song list */}
      <div className="mt-4 flex flex-1 flex-col overflow-hidden px-2">
        <div className="flex items-center justify-between px-2 pb-1 text-[11px] font-semibold tracking-wide text-[var(--color-text-dim)]">
          <span>LOCAL MUSIC</span>
          <ImportButton>
            <UploadCloud size={12} className="hover:text-white" />
          </ImportButton>
        </div>
        <SongList />
      </div>

      {/* Batch separation controls */}
      <div className="shrink-0 border-t border-[var(--color-border)] px-3 py-3">
        {isBatchRunning ? (
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <span className="text-[11px] text-[var(--color-text-dim)]">
                Separating {Math.min(batchSeparation.completed + 1, batchSeparation.total)}/{batchSeparation.total}
              </span>
              <button
                onClick={handleCancelBatch}
                className="text-[var(--color-text-dim)] transition-colors hover:text-white"
                title="Cancel batch separation"
                aria-label="Cancel batch separation"
              >
                <X size={12} />
              </button>
            </div>
            <div className="h-1.5 w-full overflow-hidden rounded-full bg-[var(--color-border)]">
              <div
                className="h-full rounded-full bg-[var(--color-accent)] transition-all"
                style={{
                  width: `${((batchSeparation.completed + (batchSeparation.current_percent ?? 0) / 100) / batchSeparation.total) * 100}%`,
                }}
              />
            </div>
            {batchSeparation.failed > 0 && (
              <span className="text-[10px] text-red-400">
                {batchSeparation.failed} failed
              </span>
            )}
          </div>
        ) : batchSeparation != null ? (
          // Completed/cancelled state (shown briefly before clearing)
          <div className="text-center text-[11px] text-[var(--color-text-dim)]">
            Separation complete: {batchSeparation.completed} done
            {batchSeparation.skipped > 0 && `, ${batchSeparation.skipped} skipped`}
            {batchSeparation.failed > 0 && `, ${batchSeparation.failed} failed`}
          </div>
        ) : (
          <button
            onClick={handleSeparateAll}
            disabled={songs.length === 0}
            className="flex w-full items-center justify-center gap-2 rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white disabled:opacity-40"
          >
            <Layers size={12} />
            Separate All
            <span className="text-[10px] text-[var(--color-text-dimmer)]">
              ({stemMode === "four_stem" ? "4-stem" : "2-stem"})
            </span>
          </button>
        )}
      </div>
    </div>
  );
}
