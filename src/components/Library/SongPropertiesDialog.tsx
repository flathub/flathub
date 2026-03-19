import { useState, useEffect, useRef } from "react";
import { useTranslation } from "react-i18next";
import * as api from "@/lib/tauri";
import { formatDuration, formatBytes } from "@/lib/format";
import {
  songCanBeSeparated,
  songSupportsInstrumentalFlag,
} from "@/lib/song-media";
import { notifyError } from "@/lib/errors";
import { useLibraryStore } from "@/stores/library-store";
import type { Song, SongProperties } from "@/types/ipc";

interface SongPropertiesDialogProps {
  song: Song;
  onClose: () => void;
}

function formatSampleRate(hz: number): string {
  return `${hz.toLocaleString()} Hz`;
}

function formatBitRate(kbps: number): string {
  return `${kbps} kbps`;
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
  const currentSong = useLibraryStore(
    (s) => s.songs.find((candidate) => candidate.hash === song.hash) ?? song,
  );
  const setSongsInstrumental = useLibraryStore((s) => s.setSongsInstrumental);
  const sepStatus = separationStatuses[currentSong.hash];
  const [showReSeparate, setShowReSeparate] = useState(false);
  const [reSeparateStemMode, setReSeparateStemMode] = useState<
    "two_stem" | "four_stem"
  >("two_stem");
  const mediaGLabel =
    currentSong.media_g_container === "zip"
      ? t("songProperties.mediaGZip")
      : currentSong.media_g_container === "paired"
        ? t("songProperties.mediaGPaired")
        : null;
  const canSeparateSong = songCanBeSeparated(currentSong);

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
            {currentSong.title || currentSong.file_path.split("/").pop()}
          </p>
          {currentSong.artist && (
            <p className="truncate text-[11px] text-[var(--color-text-dim)]">
              {currentSong.artist}
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
              {mediaGLabel && (
                <PropertyRow
                  label={t("songProperties.graphics")}
                  value={mediaGLabel}
                />
              )}
              {songSupportsInstrumentalFlag(currentSong) && (
                <div className="flex items-center justify-between gap-3 py-1.5">
                  <span className="w-28 shrink-0 text-[12px] text-[var(--color-text-dim)]">
                    {t("songProperties.instrumental")}
                  </span>
                  <label className="flex items-center gap-2 text-[12px] text-white">
                    <input
                      type="checkbox"
                      checked={currentSong.instrumental}
                      onChange={(event) =>
                        void setSongsInstrumental(
                          [currentSong.hash],
                          event.target.checked,
                        )
                      }
                      className="h-4 w-4 rounded border-[var(--color-border-light)] bg-[var(--color-surface)] accent-[var(--color-accent)]"
                    />
                  </label>
                </div>
              )}
              <div className="flex items-baseline gap-3 py-1.5">
                <span className="w-28 shrink-0 text-[12px] text-[var(--color-text-dim)]">
                  {t("songProperties.separation")}
                </span>
                <span className="flex items-center gap-2 text-[12px] text-white">
                  {mediaGLabel
                    ? t("songProperties.notApplicable")
                    : !sepStatus || sepStatus.state === "idle"
                      ? t("songProperties.notSeparated")
                      : sepStatus.state === "running"
                        ? t("songProperties.separating")
                        : sepStatus.state === "failed"
                          ? t("songProperties.separationFailed")
                          : sepStatus.drums_path
                            ? t("songProperties.fourStem")
                            : t("songProperties.twoStem")}
                  {sepStatus?.state === "completed" &&
                    canSeparateSong &&
                    !mediaGLabel &&
                    !sepStatus.drums_path && (
                      <button
                        onClick={() => {
                          api
                            .upgradeToFourStem(currentSong.hash)
                            .catch(() => {});
                        }}
                        className="ml-1 rounded bg-[var(--color-border)] px-2 py-0.5 text-[11px] text-[var(--color-text-dim)] transition-colors hover:text-white"
                      >
                        {t("songProperties.upgradeToFourStem")}
                      </button>
                    )}
                  {sepStatus?.state === "completed" &&
                    canSeparateSong &&
                    sepStatus.drums_path &&
                    !mediaGLabel && (
                      <button
                        onClick={() => {
                          api
                            .downgradeToTwoStem(currentSong.hash)
                            .then((status) => {
                              useLibraryStore
                                .getState()
                                .updateSeparationStatus(status);
                            })
                            .catch(notifyError);
                        }}
                        className="ml-1 rounded bg-[var(--color-border)] px-2 py-0.5 text-[11px] text-[var(--color-text-dim)] transition-colors hover:text-white"
                      >
                        {t("songProperties.downgradeToTwoStem")}
                      </button>
                    )}
                </span>
              </div>
              {sepStatus?.state === "completed" &&
                canSeparateSong &&
                !mediaGLabel && (
                  <div className="py-1.5 pl-[calc(7rem+0.75rem)]">
                    {!showReSeparate ? (
                      <button
                        onClick={() => {
                          setReSeparateStemMode(
                            sepStatus.drums_path ? "four_stem" : "two_stem",
                          );
                          setShowReSeparate(true);
                        }}
                        className="rounded bg-[var(--color-border)] px-2 py-0.5 text-[11px] text-[var(--color-text-dim)] transition-colors hover:text-white"
                      >
                        {t("songProperties.reSeparate")}
                      </button>
                    ) : (
                      <div className="flex items-center gap-2">
                        <span className="text-[11px] text-[var(--color-text-dim)]">
                          {t("songProperties.reSeparateAs")}
                        </span>
                        <button
                          onClick={() => setReSeparateStemMode("two_stem")}
                          className={`rounded px-2 py-0.5 text-[11px] transition-colors ${
                            reSeparateStemMode === "two_stem"
                              ? "bg-[var(--color-accent)] text-white"
                              : "bg-[var(--color-border)] text-[var(--color-text-dim)] hover:text-white"
                          }`}
                        >
                          {t("songProperties.twoStem")}
                        </button>
                        <button
                          onClick={() => setReSeparateStemMode("four_stem")}
                          className={`rounded px-2 py-0.5 text-[11px] transition-colors ${
                            reSeparateStemMode === "four_stem"
                              ? "bg-[var(--color-accent)] text-white"
                              : "bg-[var(--color-border)] text-[var(--color-text-dim)] hover:text-white"
                          }`}
                        >
                          {t("songProperties.fourStem")}
                        </button>
                        <button
                          onClick={() => {
                            setShowReSeparate(false);
                            api
                              .reSeparate(currentSong.hash, reSeparateStemMode)
                              .catch(notifyError);
                          }}
                          className="rounded bg-[var(--color-accent)] px-2 py-0.5 text-[11px] text-white transition-opacity hover:opacity-80"
                        >
                          {t("songProperties.confirm")}
                        </button>
                      </div>
                    )}
                  </div>
                )}
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
