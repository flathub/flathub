import { useState } from "react";
import { Loader2 } from "lucide-react";
import { useLibraryStore } from "@/stores/library-store";
import { usePlayerStore } from "@/stores/player-store";
import { useSettingsStore } from "@/stores/settings-store";
import { useQueueStore } from "@/stores/queue-store";
import { formatDuration } from "@/lib/format";
import * as api from "@/lib/tauri";
import { notifyError } from "@/lib/errors";
import { ContextMenu } from "./ContextMenu";
import { SongEditDialog } from "./SongEditDialog";
import { SongPropertiesDialog } from "./SongPropertiesDialog";
import type { Song } from "@/types/ipc";

interface SongListItemProps {
  song: Song;
  orderedHashes: string[];
}

export function SongListItem({ song, orderedHashes }: SongListItemProps) {
  const selectedSongIds = useLibraryStore((s) => s.selectedSongIds);
  const selectSong = useLibraryStore((s) => s.selectSong);
  const separationStatus = useLibraryStore(
    (s) => s.separationStatuses[song.hash],
  );
  const snapshot = usePlayerStore((s) => s.snapshot);
  const playSong = usePlayerStore((s) => s.playSong);
  const closeSettings = useSettingsStore((s) => s.close);

  const [contextMenu, setContextMenu] = useState<{ x: number; y: number } | null>(null);
  const [editDialogOpen, setEditDialogOpen] = useState(false);
  const [propertiesDialogOpen, setPropertiesDialogOpen] = useState(false);

  const isSelected = selectedSongIds.has(song.hash);
  const isCurrentPlaying =
    snapshot?.song_id === song.hash && snapshot?.is_playing;
  const sepState = separationStatus?.state ?? "idle";

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

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    // If right-clicking on a non-selected song, select only that song
    if (!selectedSongIds.has(song.hash)) {
      selectSong(song.hash, { shiftKey: false, metaKey: false, ctrlKey: false }, orderedHashes);
    }
    setContextMenu({ x: e.clientX, y: e.clientY });
  };

  return (
    <div
      onClick={(e) => selectSong(song.hash, { shiftKey: e.shiftKey, metaKey: e.metaKey, ctrlKey: e.ctrlKey }, orderedHashes)}
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
        </div>

        <div className="flex shrink-0 items-center gap-2">
          {sepState === "idle" && (
            <button
              onClick={handleSeparate}
              className={`rounded px-1.5 py-0.5 text-[10px] border ${
                isSelected
                  ? "border-white/30 hover:bg-white/20"
                  : "border-[var(--color-border-light)] bg-[var(--color-hover)] text-[var(--color-text-dim)] hover:bg-[var(--color-active)]"
              }`}
            >
              Separate
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
            <span className={`text-[11px] ${isSelected ? "text-white/70" : "text-[var(--color-text-dim)]"}`}>
              {formatDuration(song.duration_ms)}
            </span>
          )}
          {sepState === "failed" && (
            <button
              onClick={handleSeparate}
              className="text-[10px] text-red-400"
            >
              Retry
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
          {song.artist || "Unknown Artist"}
        </span>
      </div>

      {contextMenu && (
        <ContextMenu
          x={contextMenu.x}
          y={contextMenu.y}
          items={
            selectedSongIds.size > 1 && isSelected
              ? [
                  {
                    label: `Queue All Selected (${selectedSongIds.size})`,
                    onClick: () => {
                      const queue = useQueueStore.getState();
                      for (const id of selectedSongIds) {
                        queue.addToQueue(id);
                      }
                    },
                  },
                  {
                    label: `Separate All Selected (${selectedSongIds.size})`,
                    onClick: () => {
                      api
                        .batchSeparate([...selectedSongIds])
                        .catch(notifyError);
                    },
                  },
                ]
              : [
                  {
                    label: "Play Now",
                    onClick: () =>
                      usePlayerStore.getState().playNow(song.hash),
                  },
                  {
                    label: "Play Next",
                    onClick: () =>
                      useQueueStore.getState().playNext(song.hash),
                  },
                  {
                    label: "Add to Queue",
                    onClick: () =>
                      useQueueStore.getState().addToQueue(song.hash),
                  },
                  {
                    label: "Extract Embedded Lyrics",
                    onClick: () => {
                      api
                        .extractEmbeddedLyrics(song.hash)
                        .catch(notifyError);
                    },
                  },
                  {
                    label: "Edit Info",
                    onClick: () => setEditDialogOpen(true),
                  },
                  {
                    label: "Properties",
                    onClick: () => setPropertiesDialogOpen(true),
                  },
                ]
          }
          onClose={() => setContextMenu(null)}
        />
      )}

      {editDialogOpen && (
        <SongEditDialog
          song={song}
          onClose={() => setEditDialogOpen(false)}
        />
      )}

      {propertiesDialogOpen && (
        <SongPropertiesDialog
          song={song}
          onClose={() => setPropertiesDialogOpen(false)}
        />
      )}
    </div>
  );
}
