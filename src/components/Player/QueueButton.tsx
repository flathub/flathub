import { ListMusic } from "lucide-react";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { useQueueStore } from "@/stores/queue-store";
import type { WindowShellTier } from "@/types/ipc";

interface QueueButtonProps {
  shellTier?: WindowShellTier;
}

export function QueueButton({ shellTier = "desktop" }: QueueButtonProps = {}) {
  const { t } = useTranslation();
  const queue = useQueueStore((s) => s.queue);
  const togglePanel = useQueueStore((s) => s.togglePanel);
  const isOpen = useQueueStore((s) => s.isOpen);
  const nativeVariant = shellTier === "mac_native";

  return (
    <Tooltip label={t("queue.title")}>
      <button
        onClick={togglePanel}
        aria-label={t("queue.title")}
        className={`motion-icon-button relative flex shrink-0 items-center transition-colors ${
          nativeVariant ? "rounded-[14px] p-2.5" : "rounded-xl p-2"
        } ${
          isOpen
            ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-[var(--color-accent)] shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
            : "text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white"
        }`}
        data-queue-button-visual-variant={nativeVariant ? "native" : "default"}
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
