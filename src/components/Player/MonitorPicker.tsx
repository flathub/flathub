import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
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

interface MonitorPickerPosition {
  left: number;
  top: number;
  minWidth: number;
}

const MONITOR_PICKER_MIN_WIDTH = 220;
const MONITOR_PICKER_OFFSET = 8;
const VIEWPORT_PADDING = 12;

function getMonitorPickerPosition(
  anchorRect: DOMRect,
  viewportWidth: number,
  viewportHeight: number,
): MonitorPickerPosition {
  const minWidth = Math.max(MONITOR_PICKER_MIN_WIDTH, anchorRect.width);
  const maxLeft = Math.max(
    VIEWPORT_PADDING,
    viewportWidth - minWidth - VIEWPORT_PADDING,
  );

  return {
    left: Math.min(
      Math.max(anchorRect.right - minWidth, VIEWPORT_PADDING),
      maxLeft,
    ),
    top: Math.min(
      anchorRect.bottom + MONITOR_PICKER_OFFSET,
      viewportHeight - VIEWPORT_PADDING,
    ),
    minWidth,
  };
}

export function MonitorPicker({ onClose, anchorRef }: MonitorPickerProps) {
  const { t } = useTranslation();
  const [monitors, setMonitors] = useState<MonitorInfo[]>([]);
  const menuRef = useRef<HTMLDivElement>(null);
  const [position, setPosition] = useState<MonitorPickerPosition | null>(null);

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

  useLayoutEffect(() => {
    const updatePosition = () => {
      const anchor = anchorRef.current;
      if (!anchor) {
        setPosition(null);
        return;
      }

      setPosition(
        getMonitorPickerPosition(
          anchor.getBoundingClientRect(),
          window.innerWidth,
          window.innerHeight,
        ),
      );
    };

    updatePosition();
    window.addEventListener("resize", updatePosition);
    window.addEventListener("scroll", updatePosition, true);

    return () => {
      window.removeEventListener("resize", updatePosition);
      window.removeEventListener("scroll", updatePosition, true);
    };
  }, [anchorRef]);

  if (monitors.length === 0 || !position) return null;

  return createPortal(
    <div
      ref={menuRef}
      className="fixed z-[70] rounded-lg border border-[var(--color-border)] bg-[color-mix(in_srgb,var(--color-sidebar)_94%,transparent)] p-1 shadow-[0_20px_40px_rgba(0,0,0,0.32)] backdrop-blur-xl"
      style={{
        left: position.left,
        top: position.top,
        minWidth: position.minWidth,
      }}
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
    </div>,
    document.body,
  );
}
