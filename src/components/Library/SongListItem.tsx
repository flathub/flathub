import { useState } from "react";
import { useTranslation } from "react-i18next";
import { Loader2 } from "lucide-react";
import { useLibraryStore } from "@/stores/library-store";
import { usePlayerStore } from "@/stores/player-store";
import { useLyricsStore } from "@/stores/lyrics-store";
import { useSettingsStore } from "@/stores/settings-store";
import { useQueueStore } from "@/stores/queue-store";
import { formatDuration } from "@/lib/format";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import { ContextMenu } from "./ContextMenu";
import { ConfirmationDialog } from "../Settings/ConfirmationDialog";
import { SongEditDialog } from "./SongEditDialog";
import { SongPropertiesDialog } from "./SongPropertiesDialog";
import type { Song } from "@/types/ipc";

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
  const songs = useLibraryStore((s) => s.songs);
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
  const selectedHasSeparableSongs = selectedSongs.some(
    (candidate) => candidate.media_g_container == null,
  );
  const isMultiSelected = selectedSongIds.size > 1 && isSelected;
  const supportsEmbeddedLyrics = song.media_g_container !== "zip";

  const handleDoubleClick = () => {
    const current = usePlayerStore.getState().snapshot;
    if (current?.is_playing && current?.song_id !== song.hash) {
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
      className={`group relative flex cursor-default select-none flex-col justify-center rounded-md px-3 py-1.5 transition-colors duration-150 ${
        isSelected
          ? "bg-[var(--color-accent)] text-white"
          : "text-[var(--color-text)] hover:bg-[var(--color-hover)]"
      }`}
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2 overflow-hidden">
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
          <span className="truncate font-medium">
            {song.title || song.file_path.split("/").pop()}
          </span>
          {song.media_g_container === "paired" && (
            <span
              className={`rounded px-1.5 py-0.5 text-[9px] font-semibold uppercase tracking-[0.08em] ${
                isSelected
                  ? "bg-white/15 text-white/80"
                  : "bg-[var(--color-hover)] text-[var(--color-text-dim)]"
              }`}
            >
              CDG
            </span>
          )}
          {song.media_g_container === "zip" && (
            <span
              className={`rounded px-1.5 py-0.5 text-[9px] font-semibold uppercase tracking-[0.08em] ${
                isSelected
                  ? "bg-white/15 text-white/80"
                  : "bg-[var(--color-hover)] text-[var(--color-text-dim)]"
              }`}
            >
              ZIP+G
            </span>
          )}
        </div>

        <div className="flex shrink-0 items-center gap-2">
          {sepState === "idle" && !isMediaG && (
            <button
              onClick={handleSeparate}
              className={`rounded px-1.5 py-0.5 text-[10px] border ${
                isSelected
                  ? "border-white/30 hover:bg-white/20"
                  : "border-[var(--color-border-light)] bg-[var(--color-hover)] text-[var(--color-text-dim)] hover:bg-[var(--color-active)]"
              }`}
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
                      : "bg-[var(--color-accent)]/20 text-[var(--color-accent)]"
                    : isSelected
                      ? "bg-white/20 text-white/80"
                      : "bg-[var(--color-hover)] text-[var(--color-text-dim)]"
                }`}
              >
                {separationStatus?.drums_path ? "4" : "2"}
              </span>
              {formatDuration(song.duration_ms)}
            </span>
          )}
          {sepState === "failed" && (
            <button
              onClick={handleSeparate}
              className="text-[10px] text-red-400"
            >
              {t("common.retry")}
            </button>
          )}
          {sepState !== "idle" &&
            sepState !== "running" &&
            sepState !== "completed" &&
            sepState !== "failed" && (
              <span className="text-[11px] text-[var(--color-text-dim)]">
                {formatDuration(song.duration_ms)}
              </span>
            )}
        </div>
      </div>

      <div className="flex pl-5">
        <span
          className={`truncate text-[11px] ${isSelected ? "text-white/80" : "text-[var(--color-text-dim)]"}`}
        >
          {song.artist || t("common.unknownArtist")}
        </span>
      </div>

      {contextMenu && (
        <ContextMenu
          x={contextMenu.x}
          y={contextMenu.y}
          items={
            isMultiSelected
              ? [
                  {
                    label: t("library.queueAllSelected", {
                      count: selectedSongIds.size,
                    }),
                    onClick: () => {
                      const queue = useQueueStore.getState();
                      for (const id of selectedSongIds) {
                        queue.addToQueue(id);
                      }
                    },
                  },
                  ...(selectedHasSeparableSongs
                    ? [
                        {
                          label: t("library.separateAllSelected", {
                            count: selectedSongIds.size,
                          }),
                          onClick: () => {
                            api
                              .batchSeparate([...selectedSongIds])
                              .catch(notifyError);
                          },
                        },
                      ]
                    : []),
                  {
                    label: t("library.deleteSelected", {
                      count: selectedSongIds.size,
                    }),
                    onClick: () => setDeleteSongIds([...selectedSongIds]),
                  },
                ]
              : [
                  {
                    label: t("library.playNow"),
                    onClick: () => usePlayerStore.getState().playNow(song.hash),
                  },
                  {
                    label: t("library.playNext"),
                    onClick: () => useQueueStore.getState().playNext(song.hash),
                  },
                  {
                    label: t("library.addToQueue"),
                    onClick: () =>
                      useQueueStore.getState().addToQueue(song.hash),
                  },
                  ...(supportsEmbeddedLyrics
                    ? [
                        {
                          label: t("library.extractEmbeddedLyrics"),
                          onClick: () => {
                            api
                              .extractEmbeddedLyrics(song.hash)
                              .catch(notifyError);
                          },
                        },
                      ]
                    : []),
                  {
                    label: t("library.fetchLyricsOnline"),
                    onClick: () => {
                      api
                        .fetchLyricsOnline(song.hash)
                        .then((payload) => {
                          // If this song is currently playing, update the lyrics store
                          const currentSongId =
                            usePlayerStore.getState().snapshot?.song_id;
                          if (
                            currentSongId === song.hash &&
                            payload.lines.length > 0
                          ) {
                            useLyricsStore.getState().clear();
                            // Trigger a re-fetch to pick up the new cached lyrics
                            useLyricsStore.getState().fetchLyrics(song.hash);
                          }
                        })
                        .catch(notifyError);
                    },
                  },
                  {
                    label: t("library.editInfo"),
                    onClick: () => setEditDialogOpen(true),
                  },
                  {
                    label: t("library.properties"),
                    onClick: () => setPropertiesDialogOpen(true),
                  },
                  {
                    label: t("library.delete"),
                    onClick: () => setDeleteSongIds([song.hash]),
                  },
                ]
          }
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
