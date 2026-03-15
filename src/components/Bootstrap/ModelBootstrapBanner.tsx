import { Loader2 } from "lucide-react";
import { useTranslation } from "react-i18next";
import { useBootstrapStore } from "@/stores/bootstrap-store";
import { formatBytes } from "@/lib/format";

export function ModelBootstrapBanner() {
  const { t } = useTranslation();
  const status = useBootstrapStore((s) => s.status);

  if (!status || status.state === "ready") return null;

  return (
    <div className="animate-expand shrink-0 border-b border-[var(--color-border)] bg-[var(--color-sidebar)] px-4 py-3">
      {status.state === "pending" && (
        <div className="flex items-center justify-between">
          <span className="text-[12px] text-[var(--color-text)]">
            {t("bootstrap.modelRequired")}
          </span>
          <span className="text-[11px] text-[var(--color-text-dim)]">
            {t("bootstrap.downloadingBackground")}
          </span>
        </div>
      )}

      {status.state === "downloading" && (
        <div className="space-y-1.5">
          <div className="flex items-center justify-between text-[12px]">
            <span className="flex items-center gap-2 text-[var(--color-text)]">
              <Loader2 size={12} className="animate-spin" />
              {t("bootstrap.downloadingModel")}
            </span>
            <span className="text-[11px] text-[var(--color-text-dim)]">
              {status.downloaded_bytes != null &&
                formatBytes(status.downloaded_bytes)}
              {status.total_bytes != null &&
                ` / ${formatBytes(status.total_bytes)}`}
            </span>
          </div>
          {status.total_bytes != null && status.downloaded_bytes != null && (
            <div className="h-1 overflow-hidden rounded-full bg-[var(--color-border)]">
              <div
                className="h-full rounded-full bg-[var(--color-accent)] transition-all"
                style={{
                  width: `${(status.downloaded_bytes / status.total_bytes) * 100}%`,
                }}
              />
            </div>
          )}
        </div>
      )}

      {status.state === "failed" && (
        <div className="flex items-center justify-between">
          <span className="text-[12px] text-red-400">
            {t("bootstrap.downloadFailed", {
              error: status.error?.message || t("bootstrap.unknownError"),
            })}
          </span>
          <span className="text-[11px] text-[var(--color-text-dim)]">
            {t("bootstrap.separationUnavailable")}
          </span>
        </div>
      )}
    </div>
  );
}
