import { X, GripVertical, ChevronUp, ChevronDown } from "lucide-react";
import { useTranslation } from "react-i18next";
import { useQueueStore } from "@/stores/queue-store";
import { useLibraryStore } from "@/stores/library-store";
import { useCallback, useRef, useState } from "react";

export function QueuePanel() {
  const { t } = useTranslation();
  const queue = useQueueStore((s) => s.queue);
  const removeFromQueue = useQueueStore((s) => s.removeFromQueue);
  const reorder = useQueueStore((s) => s.reorder);
  const clearQueue = useQueueStore((s) => s.clearQueue);
  const songs = useLibraryStore((s) => s.songs);

  const [dragIndex, setDragIndex] = useState<number | null>(null);
  const [dragOverIndex, setDragOverIndex] = useState<number | null>(null);
  const dragRef = useRef<number | null>(null);

  const getSong = useCallback(
    (hash: string) => songs.find((s) => s.hash === hash),
    [songs],
  );

  const handleDragStart = (e: React.DragEvent, index: number) => {
    dragRef.current = index;
    setDragIndex(index);
    e.dataTransfer.effectAllowed = "move";
    e.dataTransfer.setData("text/plain", String(index));
  };

  const handleDragEnter = (e: React.DragEvent) => {
    e.preventDefault();
  };

  const handleDragOver = (e: React.DragEvent, index: number) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = "move";
    setDragOverIndex(index);
  };

  const handleDrop = (e: React.DragEvent, index: number) => {
    e.preventDefault();
    if (dragRef.current !== null && dragRef.current !== index) {
      reorder(dragRef.current, index);
    }
    dragRef.current = null;
    setDragIndex(null);
    setDragOverIndex(null);
  };

  const handleDragEnd = () => {
    dragRef.current = null;
    setDragIndex(null);
    setDragOverIndex(null);
  };

  return (
    <div className="flex w-[280px] shrink-0 h-full flex-col border-l border-[var(--color-border)] bg-[var(--color-toolbar)]">
      {/* Header */}
      <div className="flex items-center justify-between border-b border-[var(--color-border)] px-4 py-2">
        <span className="text-[13px] font-medium text-[#EBEBF5]">
          {t("queue.upNext")}
          {queue.length > 0 && (
            <span className="ml-2 text-[var(--color-text-dimmer)]">
              ({queue.length})
            </span>
          )}
        </span>
        {queue.length > 0 && (
          <button
            onClick={clearQueue}
            className="text-[11px] text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5]"
          >
            {t("queue.clearAll")}
          </button>
        )}
      </div>

      {/* Queue list */}
      <div className="custom-scrollbar flex-1 overflow-y-auto">
        {queue.length === 0 ? (
          <div className="flex items-center justify-center py-8">
            <span className="text-[13px] text-[var(--color-text-dimmer)]">
              {t("queue.empty")}
            </span>
          </div>
        ) : (
          queue.map((songId, index) => {
            const song = getSong(songId);
            const isDragging = dragIndex === index;
            const isDragOver = dragOverIndex === index && dragIndex !== index;
            const dropAbove =
              isDragOver && dragIndex !== null && dragIndex > index;
            const dropBelow =
              isDragOver && dragIndex !== null && dragIndex < index;

            return (
              <div
                key={songId}
                draggable
                onDragStart={(e) => handleDragStart(e, index)}
                onDragEnter={handleDragEnter}
                onDragOver={(e) => handleDragOver(e, index)}
                onDrop={(e) => handleDrop(e, index)}
                onDragEnd={handleDragEnd}
                className={`group flex items-center gap-1.5 px-3 py-1.5 transition-colors ${
                  isDragging ? "opacity-50" : "hover:bg-[var(--color-hover)]"
                } ${dropAbove ? "border-t border-t-[var(--color-accent)]" : ""} ${dropBelow ? "border-b border-b-[var(--color-accent)]" : ""}`}
              >
                <GripVertical
                  size={12}
                  className="shrink-0 cursor-grab text-[var(--color-text-dimmer)] active:cursor-grabbing"
                />
                <div className="flex shrink-0 flex-col -my-0.5">
                  <button
                    onClick={() => reorder(index, index - 1)}
                    disabled={index === 0}
                    className="text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5] disabled:opacity-20"
                    title={t("queue.moveUp")}
                    aria-label={t("queue.moveUp")}
                  >
                    <ChevronUp size={10} />
                  </button>
                  <button
                    onClick={() => reorder(index, index + 1)}
                    disabled={index === queue.length - 1}
                    className="text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5] disabled:opacity-20"
                    title={t("queue.moveDown")}
                    aria-label={t("queue.moveDown")}
                  >
                    <ChevronDown size={10} />
                  </button>
                </div>
                <div className="flex min-w-0 flex-1 flex-col">
                  <span className="truncate text-[12px] font-medium text-[#EBEBF5]">
                    {song?.title || songId.slice(0, 8)}
                  </span>
                  <span className="truncate text-[10px] text-[var(--color-text-dimmer)]">
                    {song?.artist || t("common.unknownArtist")}
                  </span>
                </div>
                <button
                  onClick={() => removeFromQueue(index)}
                  className="shrink-0 text-[var(--color-text-dimmer)] opacity-0 transition-opacity group-hover:opacity-100 hover:text-[#EBEBF5]"
                >
                  <X size={12} />
                </button>
              </div>
            );
          })
        )}
      </div>
    </div>
  );
}
