import { useRef, useState } from "react";
import { Monitor, PanelLeft, Settings } from "lucide-react";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { AirPlayRouteButton } from "@/components/Player/AirPlayRouteButton";
import { MonitorPicker } from "@/components/Player/MonitorPicker";
import { useLayoutStore } from "@/stores/layout-store";
import { useSettingsStore } from "@/stores/settings-store";

export function NativeFloatingControls() {
  const { t } = useTranslation();
  const [monitorPickerOpen, setMonitorPickerOpen] = useState(false);
  const monitorBtnRef = useRef<HTMLButtonElement>(null);
  const sidebarVisible = useLayoutStore((s) => s.sidebarVisible);
  const toggleSidebar = useLayoutStore((s) => s.toggleSidebar);
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const toggleSettings = useSettingsStore((s) => s.toggle);

  return (
    <>
      {!sidebarVisible ? (
        <div
          className="pointer-events-none absolute left-4 top-4 z-20"
          data-native-sidebar-restore="true"
        >
          <div className="pointer-events-auto rounded-[18px] border border-[var(--native-chrome-border)] bg-[var(--native-chrome-bg)] p-2 shadow-[var(--native-panel-shadow)] backdrop-blur-xl">
            <Tooltip label={t("toolbar.toggleSidebar")}>
              <button
                type="button"
                onClick={toggleSidebar}
                aria-label={t("toolbar.toggleSidebar")}
                className="motion-icon-button rounded-[14px] p-2.5 text-[var(--color-text-dim)] hover:bg-white/6 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
              >
                <PanelLeft size={18} />
              </button>
            </Tooltip>
          </div>
        </div>
      ) : null}

      <div
        className="pointer-events-none absolute right-4 top-4 z-20"
        data-native-floating-controls="true"
      >
        <div className="pointer-events-auto flex items-center gap-2 rounded-[18px] border border-[var(--native-chrome-border)] bg-[var(--native-chrome-bg)] px-2 py-2 shadow-[var(--native-panel-shadow)] backdrop-blur-xl">
          <AirPlayRouteButton />
          <div>
            <Tooltip label={t("player.selectMonitor")}>
              <button
                ref={monitorBtnRef}
                type="button"
                onClick={() => setMonitorPickerOpen((open) => !open)}
                aria-label={t("player.selectMonitor")}
                className={`motion-icon-button rounded-[14px] p-2.5 text-[var(--color-text-dim)] hover:bg-white/6 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                  monitorPickerOpen
                    ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                    : ""
                }`}
              >
                <Monitor size={18} />
              </button>
            </Tooltip>
            {monitorPickerOpen ? (
              <MonitorPicker
                onClose={() => setMonitorPickerOpen(false)}
                anchorRef={monitorBtnRef}
              />
            ) : null}
          </div>
          <Tooltip label={t("toolbar.settings")}>
            <button
              type="button"
              onClick={toggleSettings}
              aria-label={t("toolbar.settings")}
              className={`motion-icon-button rounded-[14px] p-2.5 text-[var(--color-text-dim)] hover:bg-white/6 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
                settingsOpen
                  ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
                  : ""
              }`}
            >
              <Settings size={18} />
            </button>
          </Tooltip>
        </div>
      </div>
    </>
  );
}
