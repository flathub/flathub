import { ListMusic } from "lucide-react";
import { useQueueStore } from "@/stores/queue-store";

export function QueueButton() {
  const queue = useQueueStore((s) => s.queue);
  const togglePanel = useQueueStore((s) => s.togglePanel);
  const isOpen = useQueueStore((s) => s.isOpen);

  return (
    <button
      onClick={togglePanel}
      className={`relative flex shrink-0 items-center transition-colors ${
        isOpen
          ? "text-[var(--color-accent)]"
          : "text-[var(--color-text-dim)] hover:text-white"
      }`}
      title="Queue"
    >
      <ListMusic size={16} />
      {queue.length > 0 && (
        <span className="absolute -right-1.5 -top-1.5 flex h-3.5 w-3.5 items-center justify-center rounded-full bg-[var(--color-accent)] text-[8px] font-bold text-white">
          {queue.length > 9 ? "9+" : queue.length}
        </span>
      )}
    </button>
  );
}
