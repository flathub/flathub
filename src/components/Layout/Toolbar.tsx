import { useRef, useState } from "react";
import { PanelLeft, UploadCloud, Settings, Maximize2 } from "lucide-react";
import { useTranslation } from "react-i18next";
import { ImportButton } from "@/components/Library/ImportButton";
import { MonitorPicker } from "@/components/Player/MonitorPicker";

interface ToolbarProps {
  onToggleSidebar: () => void;
  onToggleSettings: () => void;
  settingsOpen: boolean;
  sidebarVisible: boolean;
}

export function Toolbar({
  onToggleSidebar,
  onToggleSettings,
  settingsOpen,
  sidebarVisible,
}: ToolbarProps) {
  const { t } = useTranslation();
  const [monitorPickerOpen, setMonitorPickerOpen] = useState(false);
  const fullscreenBtnRef = useRef<HTMLButtonElement>(null);

  return (
    <div className="flex h-12 shrink-0 items-center border-b border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-toolbar)_92%,transparent)] px-4 shadow-[0_1px_0_rgba(255,255,255,0.02)] backdrop-blur-xl">
      <div className="flex items-center gap-4">
        {/* Reserve space for macOS traffic lights when sidebar is hidden */}
        {!sidebarVisible && <div className="w-[54px] shrink-0" />}
        <button
          onClick={onToggleSidebar}
          className="motion-icon-button rounded-md p-1.5 text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
        >
          <PanelLeft size={16} />
        </button>
        <div className="h-4 w-px bg-[var(--color-border-light)]" />
        <ImportButton>
          <span className="motion-surface flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-hover)] px-2.5 py-1 text-[12px] font-medium text-[var(--color-text)] hover:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] hover:bg-[var(--color-active)] hover:text-white">
            <UploadCloud size={14} /> {t("toolbar.import")}
          </span>
        </ImportButton>
      </div>

      <div
        className="min-w-0 flex-1 self-stretch px-4"
        data-tauri-drag-region
      />

      <div className="flex items-center gap-4">
        <button
          onClick={onToggleSettings}
          className={`motion-icon-button rounded-md p-1.5 ${
            settingsOpen
              ? "bg-[color-mix(in_srgb,var(--color-hover)_86%,transparent)] text-white shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
              : "text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white"
          }`}
        >
          <Settings size={16} />
        </button>
        <div>
          <button
            ref={fullscreenBtnRef}
            onClick={() => setMonitorPickerOpen(!monitorPickerOpen)}
            className="motion-icon-button rounded-md p-1.5 text-[var(--color-text-dim)] hover:bg-white/4 hover:text-white focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
          >
            <Maximize2 size={16} />
          </button>
          {monitorPickerOpen && (
            <MonitorPicker
              onClose={() => setMonitorPickerOpen(false)}
              anchorRef={fullscreenBtnRef}
            />
          )}
        </div>
      </div>
    </div>
  );
}
