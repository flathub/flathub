import { useRef, useState } from "react";
import { Settings, Monitor } from "lucide-react";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { AirPlayRouteButton } from "@/components/Player/AirPlayRouteButton";
import { MonitorPicker } from "@/components/Player/MonitorPicker";
import { APP_SHORTCUTS, getShortcutDisplay } from "@/lib/app-shortcuts";

interface MacMainUtilityPillProps {
  onToggleSettings: () => void;
  settingsOpen: boolean;
}

export function MacMainUtilityPill({
  onToggleSettings,
  settingsOpen,
}: MacMainUtilityPillProps) {
  const { t } = useTranslation();
  const [monitorPickerOpen, setMonitorPickerOpen] = useState(false);
  const monitorBtnRef = useRef<HTMLButtonElement>(null);

  return (
    <div
      className="app-panel-surface pointer-events-auto absolute right-3 top-3 z-20 flex items-center gap-1 rounded-xl border border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)] px-1.5 py-1 shadow-[0_8px_24px_rgba(0,0,0,0.2)] backdrop-blur-xl"
      data-mac-main-utility-pill
    >
      <Tooltip
        label={t("toolbar.settings")}
        shortcut={getShortcutDisplay(APP_SHORTCUTS.toggleSettings)}
      >
        <button
          type="button"
          onClick={onToggleSettings}
          aria-label={t("toolbar.settings")}
          className={`motion-icon-button rounded-lg p-2 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
            settingsOpen
              ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white"
              : "text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white"
          }`}
        >
          <Settings size={16} />
        </button>
      </Tooltip>
      <AirPlayRouteButton className="h-9 w-9 rounded-lg" />
      <div>
        <Tooltip label={t("player.selectMonitor")}>
          <button
            type="button"
            ref={monitorBtnRef}
            onClick={() => setMonitorPickerOpen(!monitorPickerOpen)}
            aria-label={t("player.selectMonitor")}
            className={`motion-icon-button rounded-lg p-2 text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30 ${
              monitorPickerOpen
                ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white"
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
  );
}
