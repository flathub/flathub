import { Loader2 } from "lucide-react";
import { useTranslation } from "react-i18next";
import { useBootstrapStore } from "@/stores/bootstrap-store";

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
        <div className="flex items-center justify-between text-[12px]">
          <span className="flex items-center gap-2 text-[var(--color-text)]">
            <Loader2 size={12} className="animate-spin" />
            {t("bootstrap.downloadingModel")}
          </span>
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
