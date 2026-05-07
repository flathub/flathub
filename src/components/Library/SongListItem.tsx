import { useState, type CSSProperties } from "react";
import { useTranslation } from "react-i18next";
import { Loader2 } from "lucide-react";
import { CoverArtThumbnail } from "@/components/Shared/CoverArtThumbnail";
import { useLibraryStore } from "@/stores/library-store";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useSettingsStore } from "@/stores/settings-store";
import { useQueueStore } from "@/stores/queue-store";
import { formatDuration } from "@/lib/format";
import { TaskProgressBar } from "@/components/Layout/GlobalProgressBar";
import {
  songCanBeSeparated,
  songSupportsInstrumentalFlag,
} from "@/lib/song-media";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import { ContextMenu } from "./ContextMenu";
import { ConfirmationDialog } from "../Settings/ConfirmationDialog";
import { SongEditDialog } from "./SongEditDialog";
import { SongPropertiesDialog } from "./SongPropertiesDialog";
import { buildSongListContextMenuItems } from "./song-list-item-menu";
import { SONG_LANGUAGES, type SongLanguage } from "./song-list-item-menu";
import type { Song } from "@/types/ipc";

function getSongDisplayName(song: Song): string {
  return song.title ?? song.file_path?.split("/").pop() ?? song.hash;
}

interface SongListItemProps {
  song: Song;
  orderedHashes: string[];
}

export function SongListItem({ song, orderedHashes }: SongListItemProps) {
  const { t } = useTranslation();
  const selectedSongIds = useLibraryStore((s) => s.selectedSongIds);
  const selectSong = useLibraryStore((s) => s.selectSong);
  const separationStatus = useLibraryStore(
    (s) => s.separationStatuses[song.hash],
  );
  const uploadStatus = useLibraryStore((s) => s.uploadStatuses[song.hash]);
  const songs = useLibraryStore((s) => s.songs);
  const extractEmbeddedCoverArt = useLibraryStore(
    (s) => s.extractEmbeddedCoverArt,
  );
  const setSongsInstrumental = useLibraryStore((s) => s.setSongsInstrumental);
  const setSongsLanguage = useLibraryStore((s) => s.setSongsLanguage);
  const snapshot = usePlayerStore((s) => s.snapshot);
  const playSong = usePlayerStore((s) => s.playSong);
  const closeSettings = useSettingsStore((s) => s.close);

  const [contextMenu, setContextMenu] = useState<{
    x: number;
    y: number;
  } | null>(null);
  const [editDialogOpen, setEditDialogOpen] = useState(false);
  const [propertiesDialogOpen, setPropertiesDialogOpen] = useState(false);
  const [deleteSongIds, setDeleteSongIds] = useState<string[] | null>(null);
  const [isDeleting, setIsDeleting] = useState(false);

  const isSelected = selectedSongIds.has(song.hash);
  const isCurrentPlaying =
    snapshot?.song_id === song.hash && snapshot?.is_playing;
  const sepState = separationStatus?.state ?? "idle";
  const isMediaG = song.media_g_container != null;
  const selectedSongs = songs.filter((candidate) =>
    selectedSongIds.has(candidate.hash),
  );
  const selectedHasSeparableSongs = selectedSongs.some(songCanBeSeparated);
  const selectedInstrumentalSongs = selectedSongs.filter(
    songSupportsInstrumentalFlag,
  );
  const selectedCanToggleInstrumentalSongs =
    selectedInstrumentalSongs.length > 0;
  const selectedInstrumentalState =
    selectedInstrumentalSongs.length === 0
      ? "unchecked"
      : selectedInstrumentalSongs.every((candidate) => candidate.instrumental)
        ? "checked"
        : selectedInstrumentalSongs.some((candidate) => candidate.instrumental)
          ? "mixed"
          : "unchecked";
  const canSeparateSong = songCanBeSeparated(song);
  const selectedInstrumentalSongIds = selectedInstrumentalSongs.map(
    (candidate) => candidate.hash,
  );
  const selectedLanguage: SongLanguage | null =
    selectedSongIds.size > 0
      ? (() => {
          const first = selectedSongs[0]?.language;
          if (!first || !SONG_LANGUAGES.includes(first as SongLanguage))
            return null;
          const allSame = selectedSongs.every(
            (candidate) => candidate.language === first,
          );
          return allSame ? (first as SongLanguage) : null;
        })()
      : song.language && SONG_LANGUAGES.includes(song.language as SongLanguage)
        ? (song.language as SongLanguage)
        : null;
  const selectedLanguageSongIds =
    selectedSongIds.size > 0 ? [...selectedSongIds] : [song.hash];
  const isMultiSelected = selectedSongIds.size > 1 && isSelected;
  const supportsEmbeddedLyrics = song.media_g_container !== "zip";
  const mediaGBadgeLabel =
    song.media_g_container === "paired"
      ? "CDG"
      : song.media_g_container === "zip"
        ? "ZIP+G"
        : null;

  const handleDoubleClick = () => {
    const current = usePlayerStore.getState().snapshot;
    if (current?.song_id && current.song_id !== song.hash) {
      useQueueStore.getState().addToQueue(song.hash);
    } else {
      playSong(song.hash);
    }
    closeSettings();
  };

  const handleSeparate = (e: React.MouseEvent) => {
    e.stopPropagation();
    api.separate(song.hash).catch((err) => notifyError(err));
  };

  const handleDeleteSongs = async (songIds: string[]) => {
    setIsDeleting(true);
    try {
      const result = await api.deleteSongs(songIds);
      for (const failure of result.failed) {
        notifyError(failure.error);
      }

      if (result.deleted_song_ids.length > 0) {
        useQueueStore.getState().removeSongIds(result.deleted_song_ids);
        useLibraryStore.setState((state) => ({
          selectedSongIds: new Set(
            [...state.selectedSongIds].filter(
              (id) => !result.deleted_song_ids.includes(id),
            ),
          ),
          lastClickedSongId: result.deleted_song_ids.includes(
            state.lastClickedSongId ?? "",
          )
            ? null
            : state.lastClickedSongId,
          separationStatuses: Object.fromEntries(
            Object.entries(state.separationStatuses).filter(
              ([id]) => !result.deleted_song_ids.includes(id),
            ),
          ),
          uploadStatuses: Object.fromEntries(
            Object.entries(state.uploadStatuses).filter(
              ([id]) => !result.deleted_song_ids.includes(id),
            ),
          ),
        }));
        await useLibraryStore.getState().loadLibrary();
        await usePlayerStore.getState().loadState();
        const lyricsStore = useLyricsStore.getState();
        if (
          lyricsStore.songId &&
          result.deleted_song_ids.includes(lyricsStore.songId)
        ) {
          lyricsStore.clear();
        }
      }
    } catch (error) {
      notifyError(error);
    } finally {
      setIsDeleting(false);
      setDeleteSongIds(null);
    }
  };

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    // If right-clicking on a non-selected song, select only that song
    if (!selectedSongIds.has(song.hash)) {
      selectSong(
        song.hash,
        { shiftKey: false, metaKey: false, ctrlKey: false },
        orderedHashes,
      );
    }
    setContextMenu({ x: e.clientX, y: e.clientY });
  };

  return (
    <div
      onClick={(e) =>
        selectSong(
          song.hash,
          { shiftKey: e.shiftKey, metaKey: e.metaKey, ctrlKey: e.ctrlKey },
          orderedHashes,
        )
      }
      onDoubleClick={handleDoubleClick}
      onContextMenu={handleContextMenu}
      className={`group relative flex cursor-default select-none items-center gap-2.5 rounded-[14px] border px-3 py-2.5 transition-colors duration-150 ${
        isSelected
          ? "border-[var(--sidebar-row-selected-border)] bg-[var(--sidebar-row-selected-bg)] text-white"
          : "border-transparent text-[var(--color-text)] hover:bg-[var(--sidebar-row-overlay-bg)]"
      }`}
      data-native-overlay-surface="song-row"
      data-song-list-item-variant="unified"
      style={
        {
          contentVisibility: "auto",
          containIntrinsicSize: "64px",
        } satisfies CSSProperties
      }
    >
      <CoverArtThumbnail
        songHash={song.hash}
        coverArt={song.cover_art}
        alt={`${getSongDisplayName(song)} cover art`}
        className="h-11 w-11 shrink-0"
      />

      <div className="flex min-w-0 flex-1 flex-col justify-center">
        <div className="flex items-center justify-between gap-3">
          <div className="flex min-w-0 items-center gap-2 overflow-hidden">
            {isCurrentPlaying ? (
              <div className="flex w-3 shrink-0 justify-center">
                <span className="relative flex h-2 w-2">
                  <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-white opacity-75" />
                  <span className="relative inline-flex h-2 w-2 rounded-full bg-white" />
                </span>
              </div>
            ) : (
              <div className="w-3 shrink-0" />
            )}
            <span className="truncate text-[15px] font-semibold">
              {getSongDisplayName(song)}
            </span>
          </div>

          <div className="flex shrink-0 items-center gap-2">
            {mediaGBadgeLabel && (
              <span
                className={`inline-flex h-[14px] items-center justify-center rounded px-1.5 text-[9px] font-semibold leading-none tracking-[0.08em] ${
                  isSelected
                    ? "bg-white/20 text-white/80"
                    : "bg-[var(--sidebar-row-overlay-bg)] text-[var(--color-text-dim)]"
                }`}
              >
                {mediaGBadgeLabel}
              </span>
            )}
            {sepState === "idle" && canSeparateSong && (
              <button
                onClick={handleSeparate}
                className={`rounded border px-1.5 py-0.5 text-[10px] ${
                  isSelected
                    ? "border-[var(--sidebar-control-border)] bg-[var(--sidebar-control-bg)] text-white hover:bg-[var(--sidebar-row-overlay-bg)]"
                    : "border-[var(--sidebar-control-border)] bg-[var(--sidebar-control-bg)] text-[var(--color-text-dim)] hover:bg-[var(--sidebar-row-overlay-bg)]"
                }`}
                data-native-overlay-surface="song-action"
              >
                {t("library.separate")}
              </button>
            )}
            {sepState === "running" && (
              <div
                className={`flex items-center gap-1 text-[11px] ${isSelected ? "text-white" : "text-[var(--color-text-dim)]"}`}
              >
                <Loader2 size={10} className="animate-spin" />
                <span>{separationStatus?.percent ?? 0}%</span>
              </div>
            )}
            {sepState === "completed" && (
              <span
                className={`flex items-center gap-1.5 text-[11px] ${isSelected ? "text-white/70" : "text-[var(--color-text-dim)]"}`}
              >
                <span
                  className={`inline-flex h-[14px] min-w-[14px] items-center justify-center rounded text-[9px] font-semibold leading-none ${
                    separationStatus?.drums_path
                      ? isSelected
                        ? "bg-white/20 text-white/80"
                        : "bg-[var(--sidebar-row-overlay-bg)] text-[var(--color-accent)]"
                      : isSelected
                        ? "bg-white/20 text-white/80"
                        : "bg-[var(--sidebar-row-overlay-bg)] text-[var(--color-text-dim)]"
                  }`}
                >
                  {separationStatus?.drums_path ? "4" : "2"}
                </span>
                {formatDuration(song.duration_ms)}
              </span>
            )}
            {sepState === "failed" && canSeparateSong && (
              <button
                onClick={handleSeparate}
                className="text-[10px] text-red-400"
              >
                {t("common.retry")}
              </button>
            )}
            {(isMediaG ||
              !canSeparateSong ||
              (sepState !== "idle" &&
                sepState !== "running" &&
                sepState !== "completed" &&
                sepState !== "failed")) && (
              <span
                className={`text-[11px] ${isSelected ? "text-white/70" : "text-[var(--color-text-dim)]"}`}
              >
                {formatDuration(song.duration_ms)}
              </span>
            )}
          </div>
        </div>

        {(separationStatus?.state === "running" ||
          uploadStatus?.state === "running") && (
          <div className="mt-1 space-y-1">
            {separationStatus?.state === "running" && (
              <TaskProgressBar
                label={t("progress.separating", {
                  title: getSongDisplayName(song),
                  defaultValue: `Separating: ${getSongDisplayName(song)}`,
                })}
                percent={separationStatus.percent}
              />
            )}
            {uploadStatus?.state === "running" && (
              <TaskProgressBar
                label={t("progress.uploadingToRemote", {
                  title: getSongDisplayName(song),
                  defaultValue: `Publishing to remote repository: ${getSongDisplayName(
                    song,
                  )}`,
                })}
                percent={uploadStatus.percent}
              />
            )}
          </div>
        )}

        <div className="flex pl-5">
          <span
            className={`truncate text-[12px] ${
              isSelected ? "text-white/80" : "text-[var(--color-text-dim)]"
            }`}
          >
            {song.artist || t("common.unknownArtist")}
          </span>
        </div>
      </div>

      {contextMenu && (
        <ContextMenu
          x={contextMenu.x}
          y={contextMenu.y}
          items={buildSongListContextMenuItems({
            t: (key, options) =>
              t(key as never, options as never) as unknown as string,
            isMultiSelected,
            selectedCount: selectedSongIds.size,
            selectedSongIds: [...selectedSongIds],
            selectedHasSeparableSongs,
            selectedCanToggleInstrumentalSongs,
            selectedInstrumentalState,
            selectedLanguage,
            setSelectedLanguage: (language) => {
              void setSongsLanguage(selectedLanguageSongIds, language);
            },
            supportsEmbeddedLyrics,
            queueAllSelected: () => {
              const queue = useQueueStore.getState();
              for (const id of selectedSongIds) {
                queue.addToQueue(id);
              }
            },
            separateAllSelected: () => {
              api.batchSeparate([...selectedSongIds]).catch(notifyError);
            },
            toggleSelectedInstrumental: () => {
              const nextInstrumental = selectedInstrumentalState !== "checked";
              void setSongsInstrumental(
                selectedInstrumentalSongIds,
                nextInstrumental,
              );
            },
            extractSelectedEmbeddedCoverArt: () => {
              void extractEmbeddedCoverArt([...selectedSongIds]);
            },
            deleteSelected: () => setDeleteSongIds([...selectedSongIds]),
            playNow: () => usePlayerStore.getState().playNow(song.hash),
            playNext: () => useQueueStore.getState().playNext(song.hash),
            addToQueue: () => useQueueStore.getState().addToQueue(song.hash),
            extractEmbeddedCoverArt: () => {
              void extractEmbeddedCoverArt([song.hash]);
            },
            extractEmbeddedLyrics: () => {
              api.extractEmbeddedLyrics(song.hash).catch(notifyError);
            },
            fetchLyricsOnline: () => {
              api
                .fetchLyricsOnline(song.hash)
                .then((payload) => {
                  // If this song is currently playing, update the lyrics store
                  const currentSongId =
                    usePlayerStore.getState().snapshot?.song_id;
                  if (currentSongId === song.hash && payload.lines.length > 0) {
                    useLyricsStore.getState().clear();
                    // Trigger a re-fetch to pick up the new cached lyrics
                    useLyricsStore.getState().fetchLyrics(song.hash);
                  }
                })
                .catch(notifyError);
            },
            editInfo: () => setEditDialogOpen(true),
            openProperties: () => setPropertiesDialogOpen(true),
            deleteSong: () => setDeleteSongIds([song.hash]),
          })}
          onClose={() => setContextMenu(null)}
        />
      )}

      {editDialogOpen && (
        <SongEditDialog song={song} onClose={() => setEditDialogOpen(false)} />
      )}

      {propertiesDialogOpen && (
        <SongPropertiesDialog
          song={song}
          onClose={() => setPropertiesDialogOpen(false)}
        />
      )}

      {deleteSongIds && (
        <ConfirmationDialog
          title={t("library.confirmDeleteTitle", {
            count: deleteSongIds.length,
          })}
          message={t("library.confirmDeleteMessage", {
            count: deleteSongIds.length,
          })}
          confirmLabel={
            isDeleting ? t("common.deleting") : t("library.deleteConfirm")
          }
          onCancel={() => {
            if (!isDeleting) {
              setDeleteSongIds(null);
            }
          }}
          onConfirm={() => {
            if (!isDeleting) {
              void handleDeleteSongs(deleteSongIds);
            }
          }}
        />
      )}
    </div>
  );
}
