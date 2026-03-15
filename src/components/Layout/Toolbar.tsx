import { PanelLeft, UploadCloud, Settings, Maximize2 } from "lucide-react";
import { ImportButton } from "@/components/Library/ImportButton";

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
  return (
    <div
      className="flex h-12 shrink-0 items-center justify-between border-b border-[var(--color-border)] bg-[var(--color-toolbar)] px-4"
      data-tauri-drag-region
    >
      <div className="flex items-center gap-4">
        {/* Reserve space for macOS traffic lights when sidebar is hidden */}
        {!sidebarVisible && <div className="w-[54px] shrink-0" />}
        <button
          onClick={onToggleSidebar}
          className="text-[var(--color-text-dim)] transition-colors hover:text-white"
        >
          <PanelLeft size={16} />
        </button>
        <div className="h-4 w-px bg-[var(--color-border-light)]" />
        <ImportButton>
          <span className="flex items-center gap-1.5 rounded-md border border-[var(--color-border-light)] bg-[var(--color-hover)] px-2.5 py-1 text-[12px] font-medium text-[var(--color-text)] transition-colors hover:bg-[var(--color-active)] hover:text-white">
            <UploadCloud size={14} /> Import
          </span>
        </ImportButton>
      </div>

      <div className="flex items-center gap-4">
        <button
          onClick={onToggleSettings}
          className={`rounded-md p-1.5 transition-colors ${
            settingsOpen
              ? "bg-[var(--color-hover)] text-white"
              : "text-[var(--color-text-dim)] hover:text-white"
          }`}
        >
          <Settings size={16} />
        </button>
        <button className="text-[var(--color-text-dim)] transition-colors hover:text-white">
          <Maximize2 size={16} />
        </button>
      </div>
    </div>
  );
}
