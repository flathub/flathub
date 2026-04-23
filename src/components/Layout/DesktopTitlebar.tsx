import { useEffect, useMemo, useRef, useState } from "react";
import {
  Copy,
  Minus,
  Monitor,
  PanelLeft,
  Settings,
  Square,
  UploadCloud,
  X,
} from "lucide-react";
import { useTranslation } from "react-i18next";
import { ImportButton } from "@/components/Library/ImportButton";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { AirPlayRouteButton } from "@/components/Player/AirPlayRouteButton";
import { MonitorPicker } from "@/components/Player/MonitorPicker";
import { APP_SHORTCUTS, getShortcutDisplay } from "@/lib/app-shortcuts";
import {
  createCurrentDesktopWindowController,
  createDesktopWindowActions,
  createNoopDesktopWindowController,
  syncWindowMaximizedState,
  type DesktopWindowController,
  type ResizeDirection,
} from "@/lib/window-chrome";
import {
  createDesktopTitlebarMenus,
  destroyDesktopTitlebarMenus,
  getDesktopMenuPopupPosition,
  type DesktopMenuKey,
  type DesktopMenuResources,
} from "@/runtime/window-shell-runtime";

interface DesktopTitlebarProps {
  onImportMenuAction?: () => void | Promise<void>;
  onOpenSettingsMenuAction?: () => void | Promise<void>;
  onToggleSettings: () => void;
  onToggleSidebar: () => void;
  settingsOpen: boolean;
  sidebarVisible: boolean;
  initialIsMaximized?: boolean;
  windowController?: DesktopWindowController;
}

const DESKTOP_MENU_KEYS: DesktopMenuKey[] = ["file", "edit", "window", "help"];

const RESIZE_HANDLES: Array<{
  className: string;
  direction: ResizeDirection;
}> = [
  {
    className: "fixed inset-x-3 top-0 z-[90] h-1.5 cursor-n-resize",
    direction: "North",
  },
  {
    className: "fixed inset-x-3 bottom-0 z-[90] h-1.5 cursor-s-resize",
    direction: "South",
  },
  {
    className: "fixed inset-y-12 left-0 z-[90] w-1.5 cursor-w-resize",
    direction: "West",
  },
  {
    className: "fixed inset-y-12 right-0 z-[90] w-1.5 cursor-e-resize",
    direction: "East",
  },
  {
    className: "fixed left-0 top-0 z-[90] h-3 w-3 cursor-nw-resize",
    direction: "NorthWest",
  },
  {
    className: "fixed right-0 top-0 z-[90] h-3 w-3 cursor-ne-resize",
    direction: "NorthEast",
  },
  {
    className: "fixed bottom-0 left-0 z-[90] h-3 w-3 cursor-sw-resize",
    direction: "SouthWest",
  },
  {
    className: "fixed bottom-0 right-0 z-[90] h-3 w-3 cursor-se-resize",
    direction: "SouthEast",
  },
];

function WindowControlButton({
  ariaLabel,
  children,
  className = "",
  onClick,
  title,
}: {
  ariaLabel: string;
  children: React.ReactNode;
  className?: string;
  onClick: () => void;
  title?: string;
}) {
  return (
    <button
      type="button"
      aria-label={ariaLabel}
      title={title ?? ariaLabel}
      onClick={onClick}
      className={`motion-icon-button flex h-7 w-9 items-center justify-center rounded-md text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${className}`}
    >
      {children}
    </button>
  );
}

export function DesktopTitlebar({
  onImportMenuAction = () => {},
  onOpenSettingsMenuAction = () => {},
  onToggleSettings,
  onToggleSidebar,
  settingsOpen,
  sidebarVisible,
  initialIsMaximized = false,
  windowController,
}: DesktopTitlebarProps) {
  const { t } = useTranslation();
  const [monitorPickerOpen, setMonitorPickerOpen] = useState(false);
  const [isMaximized, setIsMaximized] = useState(initialIsMaximized);
  const [menus, setMenus] = useState<DesktopMenuResources | null>(null);
  const monitorBtnRef = useRef<HTMLButtonElement>(null);
  const menuHandlersRef = useRef({
    onImportMenuAction,
    onOpenSettingsMenuAction,
  });
  const controller = useMemo(
    () =>
      windowController ??
      (typeof window === "undefined"
        ? createNoopDesktopWindowController()
        : createCurrentDesktopWindowController()),
    [windowController],
  );
  const desktopWindowActions = useMemo(
    () => createDesktopWindowActions(controller),
    [controller],
  );

  useEffect(() => {
    menuHandlersRef.current = {
      onImportMenuAction,
      onOpenSettingsMenuAction,
    };
  }, [onImportMenuAction, onOpenSettingsMenuAction]);

  useEffect(() => {
    let cancelled = false;
    let nextMenus: DesktopMenuResources | null = null;

    void createDesktopTitlebarMenus({
      onImportFiles: () => menuHandlersRef.current.onImportMenuAction(),
      onOpenSettings: () => menuHandlersRef.current.onOpenSettingsMenuAction(),
      t,
    })
      .then((createdMenus) => {
        if (cancelled) {
          void destroyDesktopTitlebarMenus(createdMenus);
          return;
        }

        nextMenus = createdMenus;
        setMenus(createdMenus);
      })
      .catch(() => {
        setMenus(null);
      });

    return () => {
      cancelled = true;
      setMenus(null);
      void destroyDesktopTitlebarMenus(nextMenus);
    };
  }, [t]);

  useEffect(() => {
    let disposed = false;
    let unlisten: (() => void) | undefined;
    const syncMaximizedState = async () => {
      await syncWindowMaximizedState(controller, setIsMaximized);
    };

    void syncMaximizedState();
    controller
      .onResized(() => {
        if (!disposed) {
          void syncMaximizedState();
        }
      })
      .then((dispose) => {
        if (disposed) {
          dispose();
          return;
        }
        unlisten = dispose;
      });

    return () => {
      disposed = true;
      unlisten?.();
    };
  }, [controller]);

  const handleOpenMenu = (key: DesktopMenuKey, target: HTMLElement) => {
    const menu = menus?.[key];
    if (!menu) {
      return;
    }

    void menu.popup(getDesktopMenuPopupPosition(target));
  };

  const maximizeLabel = isMaximized
    ? t("windowChrome.restore")
    : t("windowChrome.maximize");

  return (
    <>
      <div
        className="app-panel-surface flex h-11 shrink-0 items-center gap-3 border-b border-[color-mix(in_srgb,var(--color-border)_88%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_96%,transparent)] px-3 shadow-[0_1px_0_rgba(255,255,255,0.02)]"
        data-maximized={isMaximized}
      >
        <div className="flex shrink-0 items-center gap-3">
          <div className="flex items-center gap-2">
            <img src="/OpenKara.svg" alt="" className="h-4 w-4 shrink-0" />
            <span className="text-[12px] font-semibold tracking-[0.02em] text-[var(--color-text)]">
              {t("app.name")}
            </span>
          </div>

          <div className="flex items-center gap-1">
            <Tooltip
              label={t("toolbar.toggleSidebar")}
              shortcut={getShortcutDisplay(APP_SHORTCUTS.toggleSidebar)}
            >
              <button
                type="button"
                onClick={onToggleSidebar}
                aria-label={t("toolbar.toggleSidebar")}
                className={`motion-icon-button rounded-md p-1.5 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                  sidebarVisible
                    ? "bg-[var(--color-hover)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                    : "text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white"
                }`}
              >
                <PanelLeft size={15} />
              </button>
            </Tooltip>

            <Tooltip
              label={t("toolbar.import")}
              shortcut={getShortcutDisplay(APP_SHORTCUTS.importFiles)}
            >
              <ImportButton ariaLabel={t("toolbar.import")}>
                <span className="motion-surface flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-hover)] px-2.5 py-1 text-[12px] font-medium text-[var(--color-text)] hover:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] hover:bg-[var(--color-active)] hover:text-white">
                  <UploadCloud size={13} /> {t("toolbar.import")}
                </span>
              </ImportButton>
            </Tooltip>
          </div>

          <div className="h-4 w-px bg-[var(--color-border-light)]" />

          <div className="flex items-center gap-0.5">
            {DESKTOP_MENU_KEYS.map((key) => (
              <button
                key={key}
                type="button"
                onClick={(event) => handleOpenMenu(key, event.currentTarget)}
                className="motion-surface rounded-md px-2 py-1 text-[12px] font-medium text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
              >
                {t(`windowChrome.${key}`)}
              </button>
            ))}
          </div>
        </div>

        <div
          className="min-w-0 flex-1 self-stretch"
          data-tauri-drag-region
          onMouseDown={(event) => {
            void desktopWindowActions.handleDragRegionPointerDown({
              buttons: event.buttons,
              detail: event.detail,
            });
          }}
        />

        <div className="flex shrink-0 items-center gap-1">
          <Tooltip
            label={t("toolbar.settings")}
            shortcut={getShortcutDisplay(APP_SHORTCUTS.toggleSettings)}
          >
            <button
              type="button"
              onClick={onToggleSettings}
              aria-label={t("toolbar.settings")}
              className={`motion-icon-button rounded-md p-1.5 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                settingsOpen
                  ? "bg-[var(--color-hover)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                  : "text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white"
              }`}
            >
              <Settings size={15} />
            </button>
          </Tooltip>

          <AirPlayRouteButton className="h-[30px] w-[30px] rounded-md" />

          <div className="h-4 w-px bg-[var(--color-border-light)]" />

          <div>
            <Tooltip label={t("player.selectMonitor")}>
              <button
                type="button"
                ref={monitorBtnRef}
                onClick={() => setMonitorPickerOpen((open) => !open)}
                aria-label={t("player.selectMonitor")}
                className={`motion-icon-button rounded-md p-1.5 text-[var(--color-text-dim)] hover:bg-[var(--color-ghost-hover)] hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                  monitorPickerOpen
                    ? "bg-[var(--color-hover)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                    : ""
                }`}
              >
                <Monitor size={15} />
              </button>
            </Tooltip>
            {monitorPickerOpen && (
              <MonitorPicker
                onClose={() => setMonitorPickerOpen(false)}
                anchorRef={monitorBtnRef}
              />
            )}
          </div>

          <div className="h-4 w-px bg-[var(--color-border-light)]" />

          <div className="flex items-center gap-0.5">
            <WindowControlButton
              ariaLabel={t("windowChrome.minimize")}
              onClick={() => {
                void desktopWindowActions.minimize();
              }}
            >
              <Minus size={14} />
            </WindowControlButton>
            <WindowControlButton
              ariaLabel={maximizeLabel}
              onClick={() => {
                void desktopWindowActions.toggleMaximize();
                void syncWindowMaximizedState(controller, setIsMaximized);
              }}
            >
              {isMaximized ? <Copy size={13} /> : <Square size={12} />}
            </WindowControlButton>
            <WindowControlButton
              ariaLabel={t("windowChrome.close")}
              className="hover:bg-[#c42b1c] hover:text-white"
              onClick={() => {
                void desktopWindowActions.close();
              }}
            >
              <X size={14} />
            </WindowControlButton>
          </div>
        </div>
      </div>

      {RESIZE_HANDLES.map((handle) => (
        <button
          key={handle.direction}
          type="button"
          tabIndex={-1}
          aria-hidden="true"
          className={`${handle.className} appearance-none bg-transparent p-0 opacity-0`}
          onMouseDown={(event) => {
            event.preventDefault();
            void desktopWindowActions.startResizeDragging(handle.direction);
          }}
        />
      ))}
    </>
  );
}
