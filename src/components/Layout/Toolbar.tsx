import { useRef, useState } from "react";
import { PanelLeft, UploadCloud, Settings, Monitor } from "lucide-react";
import { useTranslation } from "react-i18next";
import { ImportButton } from "@/components/Library/ImportButton";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { AirPlayRouteButton } from "@/components/Player/AirPlayRouteButton";
import { MonitorPicker } from "@/components/Player/MonitorPicker";
import {
  createWindowShellStyle,
  getDefaultWindowShellState,
  type WindowShellState,
} from "@/lib/window-shell";
import { APP_SHORTCUTS, getShortcutDisplay } from "@/lib/app-shortcuts";

interface ToolbarProps {
  hideLeadingShellControls?: boolean;
  onToggleSidebar: () => void;
  onToggleSettings: () => void;
  shellState?: WindowShellState;
  settingsOpen: boolean;
  sidebarVisible: boolean;
}

export function Toolbar({
  hideLeadingShellControls = false,
  onToggleSidebar,
  onToggleSettings,
  shellState,
  settingsOpen,
  sidebarVisible,
}: ToolbarProps) {
  const { t } = useTranslation();
  const [monitorPickerOpen, setMonitorPickerOpen] = useState(false);
  const monitorBtnRef = useRef<HTMLButtonElement>(null);
  const resolvedShellState = shellState ?? getDefaultWindowShellState("mac");
  const macWindowChrome = resolvedShellState.chromeVariant === "mac";
  const nativeShell = resolvedShellState.tier === "mac_native";

  return (
    <div
      className={`flex shrink-0 items-center border-b px-4 shadow-[0_1px_0_rgba(255,255,255,0.02)] backdrop-blur-xl ${
        nativeShell
          ? "border-[color-mix(in_srgb,var(--color-border)_72%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_88%,transparent)]"
          : "border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)]"
      }`}
      data-window-shell-tier={resolvedShellState.tier}
      style={{
        ...createWindowShellStyle(resolvedShellState),
        height: "var(--window-shell-toolbar-height)",
      }}
    >
      {hideLeadingShellControls ? (
        <div
          className="flex items-center gap-3"
          style={
            macWindowChrome
              ? {
                  paddingInlineStart:
                    "var(--window-shell-leading-controls-space)",
                  transform: nativeShell ? undefined : "translateY(-4px)",
                }
              : undefined
          }
        />
      ) : (
        <div
          className="flex items-center gap-3"
          style={
            macWindowChrome
              ? {
                  paddingInlineStart:
                    "var(--window-shell-leading-controls-space)",
                  transform: nativeShell ? undefined : "translateY(-4px)",
                }
              : undefined
          }
        >
          <Tooltip
            label={t("toolbar.toggleSidebar")}
            shortcut={getShortcutDisplay(APP_SHORTCUTS.toggleSidebar)}
          >
            <button
              onClick={onToggleSidebar}
              aria-label={t("toolbar.toggleSidebar")}
              className={`motion-icon-button rounded-xl p-2 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                sidebarVisible
                  ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                  : "text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white"
              }`}
            >
              <PanelLeft size={16} />
            </button>
          </Tooltip>
          <div className="h-4 w-px bg-[var(--color-border-light)]" />
          <Tooltip
            label={t("toolbar.import")}
            shortcut={getShortcutDisplay(APP_SHORTCUTS.importFiles)}
          >
            <ImportButton ariaLabel={t("toolbar.import")}>
              <span className="motion-surface flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-hover)] px-2.5 py-1 text-[12px] font-medium text-[var(--color-text)] hover:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] hover:bg-[var(--color-active)] hover:text-white">
                <UploadCloud size={14} /> {t("toolbar.import")}
              </span>
            </ImportButton>
          </Tooltip>
        </div>
      )}

      <div
        className="min-w-0 flex-1 self-stretch px-4"
        data-tauri-drag-region
      />

      <div className="flex items-center gap-4">
        <Tooltip
          label={t("toolbar.settings")}
          shortcut={getShortcutDisplay(APP_SHORTCUTS.toggleSettings)}
        >
          <button
            onClick={onToggleSettings}
            aria-label={t("toolbar.settings")}
            className={`motion-icon-button rounded-xl p-2 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
              settingsOpen
                ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                : "text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white"
            }`}
          >
            <Settings size={16} />
          </button>
        </Tooltip>
        <AirPlayRouteButton />
        <div>
          <Tooltip label={t("player.selectMonitor")}>
            <button
              ref={monitorBtnRef}
              onClick={() => setMonitorPickerOpen(!monitorPickerOpen)}
              aria-label={t("player.selectMonitor")}
              className={`motion-icon-button rounded-xl p-2 text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                monitorPickerOpen
                  ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                  : ""
              }`}
            >
              <Monitor size={16} />
            </button>
          </Tooltip>
          {monitorPickerOpen && (
            <MonitorPicker
              onClose={() => setMonitorPickerOpen(false)}
              anchorRef={monitorBtnRef}
            />
          )}
        </div>
      </div>
    </div>
  );
}
