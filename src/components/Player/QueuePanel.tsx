import { X, GripVertical } from "lucide-react";
import { useQueueStore } from "@/stores/queue-store";
import { useLibraryStore } from "@/stores/library-store";
import { useCallback, useRef, useState } from "react";

export function QueuePanel() {
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

  const handleDragStart = (index: number) => {
    dragRef.current = index;
    setDragIndex(index);
  };

  const handleDragOver = (e: React.DragEvent, index: number) => {
    e.preventDefault();
    setDragOverIndex(index);
  };

  const handleDrop = (index: number) => {
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
    <div className="absolute bottom-0 left-0 right-0 z-20 border-t border-[var(--color-border)] bg-[var(--color-toolbar)]">
      {/* Header */}
      <div className="flex items-center justify-between border-b border-[var(--color-border)] px-4 py-2">
        <span className="text-[13px] font-medium text-[#EBEBF5]">
          Up Next
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
            Clear All
          </button>
        )}
      </div>

      {/* Queue list */}
      <div className="custom-scrollbar max-h-[240px] overflow-y-auto">
        {queue.length === 0 ? (
          <div className="flex items-center justify-center py-8">
            <span className="text-[13px] text-[var(--color-text-dimmer)]">
              Queue is empty
            </span>
          </div>
        ) : (
          queue.map((songId, index) => {
            const song = getSong(songId);
            const isDragging = dragIndex === index;
            const isDragOver = dragOverIndex === index && dragIndex !== index;

            return (
              <div
                key={songId}
                draggable
                onDragStart={() => handleDragStart(index)}
                onDragOver={(e) => handleDragOver(e, index)}
                onDrop={() => handleDrop(index)}
                onDragEnd={handleDragEnd}
                className={`group flex items-center gap-2 px-4 py-1.5 transition-colors ${
                  isDragging
                    ? "opacity-50"
                    : isDragOver
                      ? "bg-[var(--color-hover)]"
                      : "hover:bg-[var(--color-hover)]"
                }`}
              >
                <GripVertical
                  size={12}
                  className="shrink-0 cursor-grab text-[var(--color-text-dimmer)] active:cursor-grabbing"
                />
                <div className="flex min-w-0 flex-1 flex-col">
                  <span className="truncate text-[12px] font-medium text-[#EBEBF5]">
                    {song?.title || songId.slice(0, 8)}
                  </span>
                  <span className="truncate text-[10px] text-[var(--color-text-dimmer)]">
                    {song?.artist || "Unknown Artist"}
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
