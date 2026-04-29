import { useState } from "react";
import { useTranslation } from "react-i18next";
import {
  ChevronDown,
  ChevronUp,
  Edit2,
  Languages,
  LoaderCircle,
} from "lucide-react";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { useLyricsAutoScroll } from "@/hooks/use-lyrics-auto-scroll";
import { useAudiencePlainTextPaging } from "@/hooks/use-audience-plain-text-paging";
import { useAirPlayPendingGuard } from "@/hooks/use-airplay-pending-guard";
import { APP_SHORTCUTS, getShortcutDisplay } from "@/lib/app-shortcuts";
import {
  stepPlainTextRemotePage,
  resolvePlainTextRemoteTarget,
  type PlainTextPageDirection,
} from "@/lib/plain-text-page-controls";
import { useSettingsStore } from "@/stores/settings-store";
import { LyricLine } from "./LyricLine";
import { LyricsFontSizeControl } from "./LyricsFontSizeControl";
import { LyricsOffsetControl } from "./LyricsOffsetControl";
import { LyricsEmptyState } from "./LyricsEmptyState";
import { LyricsEditDialog } from "./LyricsEditDialog";
import {
  buildAudiencePresentationSpec,
  colorToCss,
} from "@/lib/audience-presentation";
import { useLyricsStore } from "@/stores/lyrics-store";
import {
  selectSyncDisplayPositionMs,
  usePlayerStore,
} from "@/stores/player-store";
interface LyricsPanelProps {
  presentation?: "standard" | "audience";
}

export function LyricsPanel({ presentation = "standard" }: LyricsPanelProps) {
  const { t } = useTranslation();
  const lines = useLyricsStore((s) => s.lines);
  const activeLineIndex = useLyricsStore((s) => s.activeLineIndex);
  const offsetMs = useLyricsStore((s) => s.offsetMs);
  const isLoading = useLyricsStore((s) => s.isLoading);
  const rawLrc = useLyricsStore((s) => s.rawLrc);
  const romanizedLines = useLyricsStore((s) => s.romanizedLines);
  const isRomanizing = useLyricsStore((s) => s.isRomanizing);
  const showRomanized = useLyricsStore((s) => s.showRomanized);
  const toggleRomanized = useLyricsStore((s) => s.toggleRomanized);
  const songId = usePlayerStore((s) => s.snapshot?.song_id);
  const positionMs = usePlayerStore(selectSyncDisplayPositionMs);
  const airPlayOutput = usePlayerStore((s) => s.airPlayOutput);
  const localAudienceOutputActive = usePlayerStore(
    (s) => s.localAudienceOutputActive,
  );
  const airPlayPlainTextPagePending = usePlayerStore(
    (s) => s.airPlayPlainTextPagePending,
  );
  const airPlayPlainTextPagePendingDirection = usePlayerStore(
    (s) => s.airPlayPlainTextPagePendingDirection,
  );
  const lyricsFontStep = useSettingsStore((s) => s.lyricsFontStep);
  const adjustedMs = positionMs - offsetMs;
  const [editOpen, setEditOpen] = useState(false);
  const utilityControlsPinned = offsetMs !== 0 || lyricsFontStep !== 0;
  const isAudience = presentation === "audience";
  const spaciousStageLayout = !isAudience;
  const audiencePresentationSpec =
    buildAudiencePresentationSpec(lyricsFontStep);

  const isPlainText =
    lines.length > 0 && lines.every((line) => line.time_ms === 0);
  const remotePlainTextTarget = resolvePlainTextRemoteTarget(
    airPlayOutput,
    localAudienceOutputActive,
  );
  const shouldShowRemotePageControls =
    !isAudience && isPlainText && remotePlainTextTarget !== null;
  const isAirPlayRemotePagingTarget = remotePlainTextTarget === "airplay";
  const shouldLockRemotePageControls =
    isAirPlayRemotePagingTarget && airPlayPlainTextPagePending;
  const shouldRenderAudiencePlainTextPages = isAudience && isPlainText;
  const pageIdentity = shouldRenderAudiencePlainTextPages
    ? `${songId ?? ""}:${rawLrc}:${lyricsFontStep}`
    : "local";

  const { containerRef, measurementRef, currentPageStart, visibleLines } =
    useAudiencePlainTextPaging({
      lines,
      shouldRender: shouldRenderAudiencePlainTextPages,
      pageIdentity,
      audiencePresentationSpec,
    });

  useLyricsAutoScroll(
    containerRef,
    activeLineIndex,
    isPlainText,
    lyricsFontStep,
    presentation,
    songId,
  );

  useAirPlayPendingGuard(
    songId,
    isPlainText,
    isAudience,
    isAirPlayRemotePagingTarget,
    airPlayPlainTextPagePending,
  );

  const handleRemotePageStep = (direction: PlainTextPageDirection) => {
    void stepPlainTextRemotePage(
      airPlayOutput,
      localAudienceOutputActive,
      direction,
    ).catch(() => {
      // Remote paging must not interrupt the operator's local view.
    });
  };

  if (!songId) {
    return (
      <div className="flex flex-1 items-center justify-center">
        <p
          className="text-[var(--color-text-dimmer)]"
          style={
            isAudience
              ? {
                  fontSize: audiencePresentationSpec.statusFontSizePx,
                  color: colorToCss(audiencePresentationSpec.statusTextColor),
                }
              : { fontSize: 14 }
          }
        >
          {t("lyrics.selectSong")}
        </p>
      </div>
    );
  }

  if (isLoading) {
    return (
      <div className="flex flex-1 items-center justify-center">
        <p
          className="text-[var(--color-text-dim)]"
          style={
            isAudience
              ? {
                  fontSize: audiencePresentationSpec.statusFontSizePx,
                  color: colorToCss(audiencePresentationSpec.statusTextColor),
                }
              : { fontSize: 14 }
          }
        >
          {t("lyrics.loadingLyrics")}
        </p>
      </div>
    );
  }

  if (lines.length === 0) {
    return <LyricsEmptyState presentation={presentation} />;
  }

  return (
    <div
      className="group relative flex flex-1 flex-col items-center overflow-hidden"
      data-lyrics-visual-variant={
        spaciousStageLayout ? "stage-layout" : "default"
      }
      data-native-lyrics-layout={spaciousStageLayout ? "true" : "false"}
    >
      {songId && !isAudience ? (
        <>
          <div
            className="contextual-reveal absolute right-4 top-4 z-10 flex gap-2"
            data-visible={utilityControlsPinned}
          >
            <Tooltip label={t("lyrics.romanizeTooltip")}>
              <button
                type="button"
                onClick={toggleRomanized}
                aria-label={t("lyrics.romanizeTooltip")}
                disabled={isRomanizing}
                className={`app-panel-surface motion-icon-button rounded-full border p-2 shadow-[0_16px_30px_rgba(0,0,0,0.22)] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35 ${
                  showRomanized
                    ? "border-[color-mix(in_srgb,var(--color-accent)_40%,var(--color-border-light))] bg-[color-mix(in_srgb,var(--color-accent)_18%,var(--color-sidebar))] text-[var(--color-control-primary)]"
                    : "border-[color-mix(in_srgb,var(--color-border-light)_78%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_76%,transparent)] text-[var(--color-text-dim)] hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_78%,transparent)] hover:text-[var(--color-control-primary)]"
                }`}
              >
                {isRomanizing ? (
                  <LoaderCircle size={14} className="animate-spin" />
                ) : (
                  <Languages size={14} />
                )}
              </button>
            </Tooltip>
            <Tooltip label={t("lyrics.editTooltip")}>
              <button
                type="button"
                onClick={() => setEditOpen(true)}
                aria-label={t("lyrics.editTooltip")}
                className="app-panel-surface motion-icon-button rounded-full border border-[color-mix(in_srgb,var(--color-border-light)_78%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_76%,transparent)] p-2 text-[var(--color-text-dim)] shadow-[0_16px_30px_rgba(0,0,0,0.22)] hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_78%,transparent)] hover:text-[var(--color-control-primary)] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35"
              >
                <Edit2 size={14} />
              </button>
            </Tooltip>
          </div>
          <LyricsEditDialog
            open={editOpen}
            onClose={() => setEditOpen(false)}
            songId={songId}
            existingLyrics={rawLrc || undefined}
          />
          {shouldShowRemotePageControls ? (
            <div className="pointer-events-none absolute inset-y-0 right-4 z-10 flex items-center">
              <div className="pointer-events-auto flex flex-col gap-3">
                <Tooltip
                  label={t("lyrics.previousPage")}
                  shortcut={getShortcutDisplay(APP_SHORTCUTS.lyricsPagePrev)}
                >
                  <button
                    type="button"
                    data-testid="plain-text-page-prev"
                    data-airplay-page-pending={
                      shouldLockRemotePageControls &&
                      airPlayPlainTextPagePendingDirection === "prev"
                        ? "true"
                        : "false"
                    }
                    onClick={() => handleRemotePageStep("prev")}
                    aria-label={t("lyrics.previousPage")}
                    disabled={shouldLockRemotePageControls}
                    className="app-panel-surface motion-icon-button rounded-full border border-[color-mix(in_srgb,var(--color-border-light)_78%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_76%,transparent)] p-2 text-[var(--color-text-dim)] shadow-[0_16px_30px_rgba(0,0,0,0.22)] hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_78%,transparent)] hover:text-[var(--color-control-primary)] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35"
                  >
                    {shouldLockRemotePageControls &&
                    airPlayPlainTextPagePendingDirection === "prev" ? (
                      <LoaderCircle size={16} className="animate-spin" />
                    ) : (
                      <ChevronUp size={16} />
                    )}
                  </button>
                </Tooltip>
                <Tooltip
                  label={t("lyrics.nextPage")}
                  shortcut={getShortcutDisplay(APP_SHORTCUTS.lyricsPageNext)}
                >
                  <button
                    type="button"
                    data-testid="plain-text-page-next"
                    data-airplay-page-pending={
                      shouldLockRemotePageControls &&
                      airPlayPlainTextPagePendingDirection === "next"
                        ? "true"
                        : "false"
                    }
                    onClick={() => handleRemotePageStep("next")}
                    aria-label={t("lyrics.nextPage")}
                    disabled={shouldLockRemotePageControls}
                    className="app-panel-surface motion-icon-button rounded-full border border-[color-mix(in_srgb,var(--color-border-light)_78%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_76%,transparent)] p-2 text-[var(--color-text-dim)] shadow-[0_16px_30px_rgba(0,0,0,0.22)] hover:border-[color-mix(in_srgb,var(--color-accent)_28%,var(--color-border-light))] hover:bg-[color-mix(in_srgb,var(--color-hover)_78%,transparent)] hover:text-[var(--color-control-primary)] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/35"
                  >
                    {shouldLockRemotePageControls &&
                    airPlayPlainTextPagePendingDirection === "next" ? (
                      <LoaderCircle size={16} className="animate-spin" />
                    ) : (
                      <ChevronDown size={16} />
                    )}
                  </button>
                </Tooltip>
              </div>
            </div>
          ) : null}
        </>
      ) : null}
      <div
        ref={containerRef}
        key={songId}
        data-testid="lyrics-scroll-viewport"
        className={`custom-scrollbar flex w-full flex-1 overflow-y-auto animate-[song-fade-in_var(--motion-duration-slow)_var(--motion-ease-emphasized-out)] ${
          isAudience ? "" : spaciousStageLayout ? "px-16 py-10" : "px-12 py-8"
        }`}
        style={
          isAudience
            ? {
                padding: `${audiencePresentationSpec.verticalPaddingPx}px ${audiencePresentationSpec.horizontalPaddingPx}px`,
              }
            : undefined
        }
      >
        <div
          className={`mx-auto flex w-full flex-col items-center ${
            isAudience
              ? shouldRenderAudiencePlainTextPages
                ? "min-h-full justify-start"
                : "min-h-full justify-center"
              : spaciousStageLayout
                ? "max-w-4xl gap-9"
                : "max-w-2xl gap-7"
          }`}
          style={
            isAudience
              ? {
                  maxWidth: `min(${audiencePresentationSpec.contentWidthRatio * 100}vw, ${audiencePresentationSpec.contentMaxWidthPx}px)`,
                  gap: audiencePresentationSpec.lineGapPx,
                }
              : undefined
          }
        >
          {visibleLines.map((line, idx) => {
            const absoluteIndex = shouldRenderAudiencePlainTextPages
              ? currentPageStart + idx
              : idx;

            return (
              <div
                key={`${absoluteIndex}-${line.time_ms}-${line.text}`}
                data-lyrics-line-index={absoluteIndex}
                className="w-full"
              >
                <LyricLine
                  line={line}
                  state={
                    isPlainText
                      ? "plain"
                      : absoluteIndex === activeLineIndex
                        ? "active"
                        : absoluteIndex < activeLineIndex
                          ? "past"
                          : "future"
                  }
                  adjustedMs={isPlainText ? 0 : adjustedMs}
                  presentation={presentation}
                  lyricsFontStep={lyricsFontStep}
                  romanizedText={
                    showRomanized ? romanizedLines[absoluteIndex] : undefined
                  }
                />
              </div>
            );
          })}
        </div>
      </div>
      {shouldRenderAudiencePlainTextPages ? (
        <div
          aria-hidden="true"
          className="pointer-events-none absolute inset-0 opacity-0"
        >
          <div
            className="flex h-full w-full"
            style={{
              padding: `${audiencePresentationSpec.verticalPaddingPx}px ${audiencePresentationSpec.horizontalPaddingPx}px`,
            }}
          >
            <div
              ref={measurementRef}
              className="mx-auto flex w-full flex-col items-center"
              style={{
                maxWidth: `min(${audiencePresentationSpec.contentWidthRatio * 100}vw, ${audiencePresentationSpec.contentMaxWidthPx}px)`,
                gap: audiencePresentationSpec.lineGapPx,
              }}
            >
              {lines.map((line, idx) => (
                <div
                  key={`measure-${idx}-${line.time_ms}-${line.text}`}
                  data-plain-text-page-measure-line
                  className="w-full"
                >
                  <LyricLine
                    line={line}
                    state="plain"
                    adjustedMs={0}
                    presentation={presentation}
                    lyricsFontStep={lyricsFontStep}
                    romanizedText={
                      showRomanized ? romanizedLines[idx] : undefined
                    }
                  />
                </div>
              ))}
            </div>
          </div>
        </div>
      ) : null}
      {!isAudience ? (
        <div
          className="pointer-events-none absolute inset-x-0 bottom-0 z-10 flex justify-center px-6 pb-5"
          data-visible={utilityControlsPinned}
        >
          <div className="flex items-center gap-3">
            <LyricsOffsetControl
              className="contextual-reveal pointer-events-auto"
              data-visible={utilityControlsPinned}
            />
            <LyricsFontSizeControl
              className="contextual-reveal pointer-events-auto"
              data-visible={utilityControlsPinned}
            />
          </div>
        </div>
      ) : null}
    </div>
  );
}
