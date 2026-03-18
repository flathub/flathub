import { useEffect, useMemo, useState } from "react";
import { useTranslation } from "react-i18next";
import { Disc3, FileAudio2 } from "lucide-react";
import { useLibraryStore } from "@/stores/library-store";
import * as api from "@/lib/tauri";
import { formatBytes, formatDuration } from "@/lib/format";
import type { ImportCandidateDetails } from "@/types/ipc";

function getDisplayName(path: string): string {
  const normalized = path.replace(/\\/g, "/");
  return normalized.split("/").pop() ?? normalized;
}

export function ImportCdgChoiceDialog() {
  const { t } = useTranslation();
  const pendingChoice = useLibraryStore((s) => s.pendingImportCdgChoice);
  const resolveChoice = useLibraryStore((s) => s.resolveCdgChoicePrompt);
  const [detailsByPath, setDetailsByPath] = useState<
    Record<string, ImportCandidateDetails>
  >({});

  const cdgName = useMemo(
    () => (pendingChoice ? getDisplayName(pendingChoice.cdgPath) : ""),
    [pendingChoice],
  );

  useEffect(() => {
    if (!pendingChoice) {
      return;
    }

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        resolveChoice(null);
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [pendingChoice, resolveChoice]);

  useEffect(() => {
    if (!pendingChoice) {
      return;
    }

    let cancelled = false;
    void api
      .getImportCandidateDetails(pendingChoice.audioCandidates)
      .then((details) => {
        if (cancelled) {
          return;
        }

        setDetailsByPath(
          Object.fromEntries(details.map((detail) => [detail.path, detail])),
        );
      })
      .catch(() => {
        if (!cancelled) {
          setDetailsByPath({});
        }
      });

    return () => {
      cancelled = true;
    };
  }, [pendingChoice]);

  if (!pendingChoice) {
    return null;
  }

  return (
    <div className="fixed inset-0 z-[80] flex items-center justify-center p-4">
      <div
        className="absolute inset-0 bg-black/60"
        onClick={() => resolveChoice(null)}
      />
      <div
        role="dialog"
        aria-modal="true"
        aria-labelledby="import-cdg-choice-title"
        className="relative w-full max-w-lg rounded-xl border border-[var(--color-border)] bg-[color-mix(in_srgb,var(--color-sidebar)_96%,transparent)] p-6 shadow-[0_24px_60px_rgba(0,0,0,0.36)] backdrop-blur-xl"
      >
        <div className="flex items-center gap-3">
          <div className="flex h-10 w-10 items-center justify-center rounded-full bg-[color-mix(in_srgb,var(--color-accent)_18%,transparent)] text-[var(--color-accent)]">
            <Disc3 size={18} />
          </div>
          <div>
            <h3
              id="import-cdg-choice-title"
              className="text-[16px] font-semibold text-white"
            >
              {t("library.importCdgChoice.title")}
            </h3>
            <p className="mt-1 text-[13px] text-[var(--color-text-dim)]">
              {t("library.importCdgChoice.message", { cdgName })}
            </p>
          </div>
        </div>

        <div className="mt-5 space-y-2">
          {pendingChoice.audioCandidates.map((audioPath) => {
            const audioName = getDisplayName(audioPath);
            const details = detailsByPath[audioPath];
            return (
              <button
                key={audioPath}
                onClick={() => resolveChoice(audioPath)}
                className="motion-surface flex w-full items-start gap-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-surface)] px-4 py-3 text-left hover:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_82%,transparent)] hover:text-white"
              >
                <FileAudio2
                  size={16}
                  className="mt-0.5 shrink-0 text-[var(--color-text-dim)]"
                />
                <div className="min-w-0 flex-1">
                  <div className="truncate text-[13px] font-medium text-[var(--color-text)]">
                    {audioName}
                  </div>
                  <div className="truncate text-[11px] text-[var(--color-text-dimmer)]">
                    {audioPath}
                  </div>
                  {details && (
                    <div className="mt-2 flex flex-wrap gap-2 text-[10px] text-[var(--color-text-dim)]">
                      <span className="rounded-full border border-[var(--color-border-light)] px-2 py-0.5">
                        {details.format}
                      </span>
                      {details.bit_rate != null && (
                        <span className="rounded-full border border-[var(--color-border-light)] px-2 py-0.5">
                          {details.bit_rate} kbps
                        </span>
                      )}
                      <span className="rounded-full border border-[var(--color-border-light)] px-2 py-0.5">
                        {formatBytes(details.file_size)}
                      </span>
                      {details.duration_ms != null && (
                        <span className="rounded-full border border-[var(--color-border-light)] px-2 py-0.5">
                          {formatDuration(details.duration_ms)}
                        </span>
                      )}
                    </div>
                  )}
                </div>
              </button>
            );
          })}
        </div>

        <div className="mt-5 flex justify-end gap-2">
          <button
            onClick={() => resolveChoice(null)}
            className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-4 py-2 text-[13px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
          >
            {t("library.importCdgChoice.skip")}
          </button>
        </div>
      </div>
    </div>
  );
}
