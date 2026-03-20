import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import { useTranslation } from "react-i18next";
import { Monitor } from "lucide-react";
import { getMonitors, openFullscreenPlayer } from "@/lib/fullscreen-player";
import { syncAirPlayAudienceState } from "@/lib/tauri";

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

const MONITOR_PICKER_MIN_WIDTH = 200;
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

function normalizeMonitors(
  monitors: Awaited<ReturnType<typeof getMonitors>>,
): MonitorInfo[] {
  return monitors.map((monitor) => ({
    name: monitor.name ?? "Display",
    width: monitor.size.width,
    height: monitor.size.height,
  }));
}

export function MonitorPicker({ onClose, anchorRef }: MonitorPickerProps) {
  const { t } = useTranslation();
  const [monitors, setMonitors] = useState<MonitorInfo[]>([]);
  const [position, setPosition] = useState<MonitorPickerPosition | null>(null);
  const menuRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    let cancelled = false;

    getMonitors()
      .then((next) => {
        if (!cancelled) {
          setMonitors(normalizeMonitors(next));
        }
      })
      .catch(() => {
        if (!cancelled) {
          setMonitors([]);
        }
      });

    return () => {
      cancelled = true;
    };
  }, []);

  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (
        menuRef.current &&
        !menuRef.current.contains(event.target as Node) &&
        anchorRef.current &&
        !anchorRef.current.contains(event.target as Node)
      ) {
        onClose();
      }
    };
    const handleEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        onClose();
      }
    };

    document.addEventListener("mousedown", handleClickOutside);
    document.addEventListener("keydown", handleEscape);

    return () => {
      document.removeEventListener("mousedown", handleClickOutside);
      document.removeEventListener("keydown", handleEscape);
    };
  }, [anchorRef, onClose]);

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

  if (!position) {
    return null;
  }

  const handleMonitorSelect = async (index: number) => {
    await syncAirPlayAudienceState({
      mode: "idle",
      songId: null,
      isPlaying: false,
      positionMs: 0,
      lines: [],
      activeLineIndex: -1,
      offsetMs: 0,
      isLoading: false,
      lyricsFontStep: 0,
      messages: {
        selectSong: t("lyrics.selectSong"),
        loadingLyrics: t("lyrics.loadingLyrics"),
        noLyrics: t("lyrics.noLyrics"),
        addLyrics: t("lyrics.addLyrics"),
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
    }).catch(() => {
      // Prefer keeping local output working even if AirPlay sync is stale.
    });

    openFullscreenPlayer(index);
    onClose();
  };

  return createPortal(
    <div
      ref={menuRef}
      className="app-panel-surface fixed z-[70] rounded-lg border border-[var(--color-border)] bg-[color-mix(in_srgb,var(--color-sidebar)_94%,transparent)] p-1 shadow-[0_20px_40px_rgba(0,0,0,0.32)]"
      style={{
        left: position.left,
        top: position.top,
        minWidth: position.minWidth,
      }}
    >
      <div className="px-2 py-1 text-[11px] font-semibold text-[var(--color-text-dim)]">
        {t("player.selectMonitor")}
      </div>
      <div className="px-2 py-1 text-[11px] font-semibold text-[var(--color-text-dim)]">
        {t("player.localDisplayOutput")}
      </div>
      {monitors.length === 0 ? (
        <div className="px-2 py-2 text-[11px] text-[var(--color-text-dim)]">
          {t("player.noDisplaysFound")}
        </div>
      ) : null}
      {monitors.map((monitor, index) => (
        <button
          key={`${monitor.name}-${monitor.width}-${monitor.height}-${index}`}
          type="button"
          onClick={() => {
            void handleMonitorSelect(index);
          }}
          className="flex w-full items-center gap-2 rounded-md px-2 py-1.5 text-left text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
        >
          <Monitor size={14} className="text-[var(--color-text-dim)]" />
          <div className="min-w-0 flex-1">
            <div className="truncate">{monitor.name}</div>
            <div className="text-[10px] text-[var(--color-text-dimmer)]">
              {t("player.monitor", { index: index + 1 })}
            </div>
          </div>
          <span className="ml-auto text-[10px] text-[var(--color-text-dimmer)]">
            {monitor.width}x{monitor.height}
          </span>
        </button>
      ))}
    </div>,
    document.body,
  );
}
