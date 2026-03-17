import { useState, useEffect, useRef } from "react";
import { useTranslation } from "react-i18next";
import { Monitor } from "lucide-react";
import { getMonitors, openFullscreenPlayer } from "@/lib/fullscreen-player";

interface MonitorInfo {
  name: string;
  width: number;
  height: number;
}

interface MonitorPickerProps {
  onClose: () => void;
  anchorRef: React.RefObject<HTMLButtonElement | null>;
}

export function MonitorPicker({ onClose, anchorRef }: MonitorPickerProps) {
  const { t } = useTranslation();
  const [monitors, setMonitors] = useState<MonitorInfo[]>([]);
  const menuRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    getMonitors()
      .then((ms) =>
        setMonitors(
          ms.map((m) => ({
            name: m.name ?? "Display",
            width: m.size.width,
            height: m.size.height,
          })),
        ),
      )
      .catch(() => {});
  }, []);

  useEffect(() => {
    const handleClickOutside = (e: MouseEvent) => {
      if (
        menuRef.current &&
        !menuRef.current.contains(e.target as Node) &&
        anchorRef.current &&
        !anchorRef.current.contains(e.target as Node)
      ) {
        onClose();
      }
    };
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    document.addEventListener("mousedown", handleClickOutside);
    document.addEventListener("keydown", handleEscape);
    return () => {
      document.removeEventListener("mousedown", handleClickOutside);
      document.removeEventListener("keydown", handleEscape);
    };
  }, [onClose, anchorRef]);

  if (monitors.length === 0) return null;

  return (
    <div
      ref={menuRef}
      className="absolute right-0 top-full z-50 mt-1 min-w-[200px] rounded-lg border border-[var(--color-border)] bg-[var(--color-sidebar)] p-1 shadow-xl"
    >
      <div className="px-2 py-1 text-[11px] font-semibold text-[var(--color-text-dim)]">
        {t("player.selectMonitor")}
      </div>
      {monitors.map((monitor, index) => (
        <button
          key={index}
          onClick={() => {
            openFullscreenPlayer(index);
            onClose();
          }}
          className="flex w-full items-center gap-2 rounded-md px-2 py-1.5 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
        >
          <Monitor size={14} className="text-[var(--color-text-dim)]" />
          <span>{t("player.monitor", { index: index + 1 })}</span>
          <span className="ml-auto text-[10px] text-[var(--color-text-dimmer)]">
            {monitor.width}x{monitor.height}
          </span>
        </button>
      ))}
    </div>
  );
}
