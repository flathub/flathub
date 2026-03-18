import { ListMusic } from "lucide-react";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { useQueueStore } from "@/stores/queue-store";

export function QueueButton() {
  const { t } = useTranslation();
  const queue = useQueueStore((s) => s.queue);
  const togglePanel = useQueueStore((s) => s.togglePanel);
  const isOpen = useQueueStore((s) => s.isOpen);

  return (
    <Tooltip label={t("queue.title")}>
      <button
        onClick={togglePanel}
        aria-label={t("queue.title")}
        className={`motion-icon-button relative flex shrink-0 items-center rounded-xl p-2 transition-colors ${
          isOpen
            ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-[var(--color-accent)] shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
            : "text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white"
        }`}
      >
        <ListMusic size={16} />
        {queue.length > 0 && (
          <span className="absolute -right-1.5 -top-1.5 flex h-3.5 w-3.5 items-center justify-center rounded-full bg-[var(--color-accent)] text-[8px] font-bold text-white">
            {queue.length > 9 ? "9+" : queue.length}
          </span>
        )}
      </button>
    </Tooltip>
  );
}
