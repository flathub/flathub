import {
  DndContext,
  KeyboardSensor,
  PointerSensor,
  closestCenter,
  useSensor,
  useSensors,
  type DragEndEvent,
} from "@dnd-kit/core";
import {
  SortableContext,
  sortableKeyboardCoordinates,
  useSortable,
  verticalListSortingStrategy,
} from "@dnd-kit/sortable";
import { CSS } from "@dnd-kit/utilities";
import { ChevronDown, ChevronUp, GripVertical, X } from "lucide-react";
import { useTranslation } from "react-i18next";
import { useCallback, useMemo, type CSSProperties } from "react";
import { useLibraryStore } from "@/stores/library-store";
import { useQueueStore } from "@/stores/queue-store";

interface SortableQueueItemProps {
  songId: string;
  index: number;
  queueLength: number;
  title: string;
  artist: string;
  moveUpLabel: string;
  moveDownLabel: string;
  dragLabel: string;
  onMoveUp: () => void;
  onMoveDown: () => void;
  onRemove: () => void;
}

function SortableQueueItem({
  songId,
  index,
  queueLength,
  title,
  artist,
  moveUpLabel,
  moveDownLabel,
  dragLabel,
  onMoveUp,
  onMoveDown,
  onRemove,
}: SortableQueueItemProps) {
  const {
    attributes,
    listeners,
    setActivatorNodeRef,
    setNodeRef,
    transform,
    transition,
    isDragging,
  } = useSortable({ id: songId });

  const style: CSSProperties = {
    transform: CSS.Transform.toString(transform),
    transition,
  };

  return (
    <div
      ref={setNodeRef}
      style={style}
      className={`group flex items-center gap-1.5 px-3 py-1.5 transition-colors ${
        isDragging
          ? "z-10 rounded-md bg-[var(--color-hover)] opacity-70 ring-1 ring-[var(--color-accent)]"
          : "hover:bg-[var(--color-hover)]"
      }`}
    >
      <button
        type="button"
        ref={setActivatorNodeRef}
        {...attributes}
        {...listeners}
        className="shrink-0 cursor-grab touch-none text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5] active:cursor-grabbing"
        title={dragLabel}
        aria-label={dragLabel}
      >
        <GripVertical size={12} />
      </button>
      <div className="-my-0.5 flex shrink-0 flex-col">
        <button
          type="button"
          onClick={onMoveUp}
          disabled={index === 0}
          className="text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5] disabled:opacity-20"
          title={moveUpLabel}
          aria-label={moveUpLabel}
        >
          <ChevronUp size={10} />
        </button>
        <button
          type="button"
          onClick={onMoveDown}
          disabled={index === queueLength - 1}
          className="text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5] disabled:opacity-20"
          title={moveDownLabel}
          aria-label={moveDownLabel}
        >
          <ChevronDown size={10} />
        </button>
      </div>
      <div className="flex min-w-0 flex-1 flex-col">
        <span className="truncate text-[12px] font-medium text-[#EBEBF5]">
          {title}
        </span>
        <span className="truncate text-[10px] text-[var(--color-text-dimmer)]">
          {artist}
        </span>
      </div>
      <button
        type="button"
        onClick={onRemove}
        className="shrink-0 text-[var(--color-text-dimmer)] opacity-0 transition-opacity group-hover:opacity-100 hover:text-[#EBEBF5]"
      >
        <X size={12} />
      </button>
    </div>
  );
}

export function QueuePanel() {
  const { t } = useTranslation();
  const queue = useQueueStore((s) => s.queue);
  const removeFromQueue = useQueueStore((s) => s.removeFromQueue);
  const reorder = useQueueStore((s) => s.reorder);
  const reorderBySongId = useQueueStore((s) => s.reorderBySongId);
  const clearQueue = useQueueStore((s) => s.clearQueue);
  const songs = useLibraryStore((s) => s.songs);

  const sensors = useSensors(
    useSensor(PointerSensor, {
      activationConstraint: {
        distance: 4,
      },
    }),
    useSensor(KeyboardSensor, {
      coordinateGetter: sortableKeyboardCoordinates,
    }),
  );

  const getSong = useCallback(
    (hash: string) => songs.find((song) => song.hash === hash),
    [songs],
  );

  const getSongLabel = useCallback(
    (songId: string) => getSong(songId)?.title || songId.slice(0, 8),
    [getSong],
  );

  const accessibility = useMemo(
    () => ({
      screenReaderInstructions: {
        draggable: String(t("queue.dragInstructions")),
      },
      announcements: {
        onDragStart({ active }: { active: { id: string | number } }) {
          return String(
            t("queue.dragStart", {
              title: getSongLabel(String(active.id)),
            }),
          );
        },
        onDragOver({
          active,
          over,
        }: {
          active: { id: string | number };
          over: { id: string | number } | null;
        }) {
          if (!over) {
            return String(t("queue.dragCancel"));
          }

          return String(
            t("queue.dragOver", {
              title: getSongLabel(String(active.id)),
              overTitle: getSongLabel(String(over.id)),
            }),
          );
        },
        onDragEnd({
          active,
          over,
        }: {
          active: { id: string | number };
          over: { id: string | number } | null;
        }) {
          if (!over) {
            return String(t("queue.dragCancel"));
          }

          return String(
            t("queue.dragEnd", {
              title: getSongLabel(String(active.id)),
              overTitle: getSongLabel(String(over.id)),
            }),
          );
        },
        onDragCancel() {
          return String(t("queue.dragCancel"));
        },
      },
    }),
    [getSongLabel, t],
  );

  const handleDragEnd = useCallback(
    (event: DragEndEvent) => {
      const { active, over } = event;

      if (!over || active.id === over.id) {
        return;
      }

      reorderBySongId(String(active.id), String(over.id));
    },
    [reorderBySongId],
  );

  return (
    <div className="flex h-full w-[280px] shrink-0 flex-col border-l border-[var(--color-border)] bg-[var(--color-toolbar)]">
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
            type="button"
            onClick={clearQueue}
            className="text-[11px] text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5]"
          >
            {t("queue.clearAll")}
          </button>
        )}
      </div>

      <div className="custom-scrollbar flex-1 overflow-y-auto">
        {queue.length === 0 ? (
          <div className="flex items-center justify-center py-8">
            <span className="text-[13px] text-[var(--color-text-dimmer)]">
              {t("queue.empty")}
            </span>
          </div>
        ) : (
          <DndContext
            sensors={sensors}
            collisionDetection={closestCenter}
            accessibility={accessibility}
            onDragEnd={handleDragEnd}
          >
            <SortableContext
              items={queue}
              strategy={verticalListSortingStrategy}
            >
              {queue.map((songId, index) => {
                const song = getSong(songId);

                return (
                  <SortableQueueItem
                    key={songId}
                    songId={songId}
                    index={index}
                    queueLength={queue.length}
                    title={song?.title || songId.slice(0, 8)}
                    artist={song?.artist || t("common.unknownArtist")}
                    moveUpLabel={t("queue.moveUp")}
                    moveDownLabel={t("queue.moveDown")}
                    dragLabel={String(
                      t("queue.reorder", {
                        title: song?.title || songId.slice(0, 8),
                      }),
                    )}
                    onMoveUp={() => reorder(index, index - 1)}
                    onMoveDown={() => reorder(index, index + 1)}
                    onRemove={() => removeFromQueue(index)}
                  />
                );
              })}
            </SortableContext>
          </DndContext>
        )}
      </div>
    </div>
  );
}
