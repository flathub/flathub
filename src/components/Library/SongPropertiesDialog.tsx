import { useState, useEffect, useRef } from "react";
import { useTranslation } from "react-i18next";
import * as api from "@/lib/tauri";
import { formatDuration, formatBytes } from "@/lib/format";
import { useLibraryStore } from "@/stores/library-store";
import type { Song, SongProperties } from "@/types/ipc";

interface SongPropertiesDialogProps {
  song: Song;
  onClose: () => void;
}

function formatSampleRate(hz: number): string {
  return `${hz.toLocaleString()} Hz`;
}

function formatBitRate(bps: number): string {
  return `${Math.round(bps / 1000)} kbps`;
}

function truncateHash(hash: string): string {
  return `${hash.slice(0, 8)}…${hash.slice(-8)}`;
}

interface PropertyRowProps {
  label: string;
  value: string;
  title?: string;
  mono?: boolean;
}

function PropertyRow({ label, value, title, mono }: PropertyRowProps) {
  return (
    <div className="flex items-baseline gap-3 py-1.5">
      <span className="w-28 shrink-0 text-[12px] text-[var(--color-text-dim)]">
        {label}
      </span>
      <span
        className={`text-[12px] text-white ${mono ? "font-mono text-[11px]" : ""}`}
        title={title}
      >
        {value}
      </span>
    </div>
  );
}

export function SongPropertiesDialog({
  song,
  onClose,
}: SongPropertiesDialogProps) {
  const { t } = useTranslation();
  const [properties, setProperties] = useState<SongProperties | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const backdropRef = useRef<HTMLDivElement>(null);
  const separationStatuses = useLibraryStore((s) => s.separationStatuses);
  const sepStatus = separationStatuses[song.hash];

  useEffect(() => {
    api
      .getSongProperties(song.hash)
      .then((props) => {
        setProperties(props);
        setLoading(false);
      })
      .catch((err) => {
        setError(err?.message ?? t("songProperties.failedToLoad"));
        setLoading(false);
      });
  }, [song.hash, t]);

  useEffect(() => {
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", handleEscape);
    return () => window.removeEventListener("keydown", handleEscape);
  }, [onClose]);

  const handleBackdropClick = (e: React.MouseEvent) => {
    if (e.target === backdropRef.current) onClose();
  };

  return (
    <div
      ref={backdropRef}
      onClick={handleBackdropClick}
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/50"
    >
      <div className="w-full max-w-sm rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] shadow-2xl">
        {/* Header */}
        <div className="flex items-center justify-between border-b border-[var(--color-border)] px-5 py-3">
          <h3 className="text-[14px] font-semibold text-white">
            {t("songProperties.title")}
          </h3>
          <button
            onClick={onClose}
            className="rounded p-0.5 text-[var(--color-text-dim)] transition-colors hover:text-white"
            aria-label={t("common.close")}
          >
            <svg
              width="14"
              height="14"
              viewBox="0 0 14 14"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
            >
              <line x1="1" y1="1" x2="13" y2="13" />
              <line x1="13" y1="1" x2="1" y2="13" />
            </svg>
          </button>
        </div>

        {/* Song title/artist */}
        <div className="border-b border-[var(--color-border)] px-5 py-3">
          <p className="truncate text-[13px] font-medium text-white">
            {song.title || song.file_path.split("/").pop()}
          </p>
          {song.artist && (
            <p className="truncate text-[11px] text-[var(--color-text-dim)]">
              {song.artist}
            </p>
          )}
        </div>

        {/* Properties grid */}
        <div className="px-5 py-3">
          {loading && (
            <p className="py-4 text-center text-[12px] text-[var(--color-text-dim)]">
              {t("common.loading")}
            </p>
          )}

          {error && (
            <p className="py-4 text-center text-[12px] text-red-400">{error}</p>
          )}

          {properties && (
            <div className="divide-y divide-[var(--color-border)]/50">
              <PropertyRow
                label={t("songProperties.format")}
                value={properties.format}
              />
              <PropertyRow
                label={t("songProperties.duration")}
                value={formatDuration(properties.duration_ms)}
              />
              {properties.sample_rate != null && (
                <PropertyRow
                  label={t("songProperties.sampleRate")}
                  value={formatSampleRate(properties.sample_rate)}
                />
              )}
              {properties.channels != null && (
                <PropertyRow
                  label={t("songProperties.channels")}
                  value={
                    properties.channels === 1
                      ? t("songProperties.channelsMono")
                      : properties.channels === 2
                        ? t("songProperties.channelsStereo")
                        : t("songProperties.channelsOther", {
                            count: properties.channels,
                          })
                  }
                />
              )}
              {properties.bit_rate != null && (
                <PropertyRow
                  label={t("songProperties.bitRate")}
                  value={formatBitRate(properties.bit_rate)}
                />
              )}
              <PropertyRow
                label={t("songProperties.fileSize")}
                value={formatBytes(properties.file_size)}
              />
              <PropertyRow
                label={t("songProperties.sha256")}
                value={truncateHash(properties.hash)}
                title={properties.hash}
                mono
              />
              <div className="flex items-baseline gap-3 py-1.5">
                <span className="w-28 shrink-0 text-[12px] text-[var(--color-text-dim)]">
                  {t("songProperties.separation")}
                </span>
                <span className="flex items-center gap-2 text-[12px] text-white">
                  {(!sepStatus || sepStatus.state === "idle") &&
                    t("songProperties.notSeparated")}
                  {sepStatus?.state === "running" &&
                    t("songProperties.separating")}
                  {sepStatus?.state === "failed" &&
                    t("songProperties.separationFailed")}
                  {sepStatus?.state === "completed" &&
                    (sepStatus.drums_path
                      ? t("songProperties.fourStem")
                      : t("songProperties.twoStem"))}
                  {sepStatus?.state === "completed" &&
                    !sepStatus.drums_path && (
                      <button
                        onClick={() => {
                          api.upgradeToFourStem(song.hash).catch(() => {});
                        }}
                        className="ml-1 rounded bg-[var(--color-border)] px-2 py-0.5 text-[11px] text-[var(--color-text-dim)] transition-colors hover:text-white"
                      >
                        {t("songProperties.upgradeToFourStem")}
                      </button>
                    )}
                  {sepStatus?.state === "completed" && sepStatus.drums_path && (
                    <button
                      onClick={() => {
                        api.reSeparate(song.hash, "two_stem").catch(() => {});
                      }}
                      className="ml-1 rounded bg-[var(--color-border)] px-2 py-0.5 text-[11px] text-[var(--color-text-dim)] transition-colors hover:text-white"
                    >
                      {t("songProperties.reSeparateAsTwoStem")}
                    </button>
                  )}
                </span>
              </div>
            </div>
          )}
        </div>

        {/* Footer */}
        <div className="flex justify-end border-t border-[var(--color-border)] px-5 py-3">
          <button
            onClick={onClose}
            className="rounded-md px-3 py-1.5 text-[12px] text-[var(--color-text-dim)] transition-colors hover:text-white"
          >
            {t("common.close")}
          </button>
        </div>
      </div>
    </div>
  );
}
