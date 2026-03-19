import { useTranslation } from "react-i18next";
import { X } from "lucide-react";
import { useLibraryStore } from "@/stores/library-store";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import { formatBytes } from "@/lib/format";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";

interface ActiveTask {
  key: string;
  label: string;
  detail?: string;
  percent: number;
  onCancel?: () => void;
}

function useActiveTasks(): ActiveTask[] {
  const { t } = useTranslation();
  const bootstrapStatus = useBootstrapStore((s) => s.status);
  const separationStatuses = useLibraryStore((s) => s.separationStatuses);
  const batchSeparation = useLibraryStore((s) => s.batchSeparation);
  const songs = useLibraryStore((s) => s.songs);

  const tasks: ActiveTask[] = [];

  // Model download
  if (bootstrapStatus?.state === "downloading") {
    const percent =
      bootstrapStatus.total_bytes && bootstrapStatus.downloaded_bytes
        ? (bootstrapStatus.downloaded_bytes / bootstrapStatus.total_bytes) * 100
        : 0;
    tasks.push({
      key: "model-download",
      label: t("bootstrap.downloadingModel"),
      detail:
        bootstrapStatus.downloaded_bytes != null
          ? formatBytes(bootstrapStatus.downloaded_bytes) +
            (bootstrapStatus.total_bytes != null
              ? ` / ${formatBytes(bootstrapStatus.total_bytes)}`
              : "")
          : undefined,
      percent,
    });
  }

  // Batch separation
  if (batchSeparation != null) {
    const done = batchSeparation.completed + batchSeparation.failed;
    if (done < batchSeparation.total) {
      const percent =
        ((batchSeparation.completed +
          (batchSeparation.current_percent ?? 0) / 100) /
          batchSeparation.total) *
        100;
      tasks.push({
        key: "batch-separation",
        label: t("sidebar.separating", {
          current: Math.min(
            batchSeparation.completed + 1,
            batchSeparation.total,
          ),
          total: batchSeparation.total,
        }),
        percent,
        onCancel: () => api.cancelBatchSeparation().catch(notifyError),
      });
    }
  }

  // Single-song separation (only when NOT part of a batch)
  if (batchSeparation == null) {
    const runningSep = Object.values(separationStatuses).find(
      (s) => s.state === "running",
    );
    if (runningSep) {
      const song = songs.find((s) => s.hash === runningSep.song_id);
      const title = song?.title ?? song?.file_path.split("/").pop() ?? "";
      tasks.push({
        key: `sep-${runningSep.song_id}`,
        label: t("progress.separating", { title }),
        percent: runningSep.percent,
      });
    }
  }

  return tasks;
}

export function GlobalProgressBar() {
  const tasks = useActiveTasks();

  if (tasks.length === 0) return null;

  return (
    <div className="app-panel-surface shrink-0 space-y-2 border-t border-[color-mix(in_srgb,var(--color-border)_86%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_94%,transparent)] px-3 py-2">
      {tasks.map((task) => (
        <div key={task.key} className="space-y-1">
          <div className="flex items-center justify-between">
            <span className="min-w-0 truncate text-[11px] text-[var(--color-text-dim)]">
              {task.label}
              {task.detail && (
                <span className="ml-1 text-[var(--color-text-dimmer)]">
                  {task.detail}
                </span>
              )}
            </span>
            {task.onCancel && (
              <button
                onClick={task.onCancel}
                className="motion-icon-button shrink-0 rounded p-0.5 text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
              >
                <X size={12} />
              </button>
            )}
          </div>
          <div className="h-1.5 w-full overflow-hidden rounded-full bg-[var(--color-border)]">
            <div
              className="motion-surface h-full rounded-full bg-[var(--color-accent)]"
              style={{ width: `${task.percent}%` }}
            />
          </div>
        </div>
      ))}
    </div>
  );
}
