import {
  getShortcutPlatform,
  type ShortcutPlatform,
} from "@/lib/app-shortcuts";
import { getWindowChromeVariant } from "@/lib/window-chrome";
import type { WindowShellState } from "@/lib/window-shell";
import { DesktopTitlebar } from "./DesktopTitlebar";
import { Toolbar } from "./Toolbar";

interface WindowChromeProps {
  onImportMenuAction?: () => void | Promise<void>;
  onOpenSettingsMenuAction?: () => void | Promise<void>;
  onToggleSettings: () => void;
  onToggleSidebar: () => void;
  settingsOpen: boolean;
  shellState?: WindowShellState;
  sidebarVisible: boolean;
  platform?: ShortcutPlatform;
}

export function WindowChrome({
  onImportMenuAction,
  onOpenSettingsMenuAction,
  onToggleSettings,
  onToggleSidebar,
  platform = getShortcutPlatform(),
  shellState,
  settingsOpen,
  sidebarVisible,
}: WindowChromeProps) {
  const chromeVariant =
    shellState?.chromeVariant ?? getWindowChromeVariant(platform);

  if (chromeVariant === "mac") {
    return (
      <Toolbar
        onToggleSidebar={onToggleSidebar}
        onToggleSettings={onToggleSettings}
        shellState={shellState}
        settingsOpen={settingsOpen}
        sidebarVisible={sidebarVisible}
      />
    );
  }

  return (
    <DesktopTitlebar
      onImportMenuAction={onImportMenuAction}
      onOpenSettingsMenuAction={onOpenSettingsMenuAction}
      onToggleSidebar={onToggleSidebar}
      onToggleSettings={onToggleSettings}
      settingsOpen={settingsOpen}
      sidebarVisible={sidebarVisible}
    />
  );
}
