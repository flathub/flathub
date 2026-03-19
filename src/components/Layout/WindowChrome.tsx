import {
  getShortcutPlatform,
  type ShortcutPlatform,
} from "@/lib/app-shortcuts";
import { getWindowChromeVariant } from "@/lib/window-chrome";
import { DesktopTitlebar } from "./DesktopTitlebar";
import { Toolbar } from "./Toolbar";

interface WindowChromeProps {
  onImportMenuAction?: () => void | Promise<void>;
  onOpenSettingsMenuAction?: () => void | Promise<void>;
  onToggleSettings: () => void;
  onToggleSidebar: () => void;
  settingsOpen: boolean;
  sidebarVisible: boolean;
  platform?: ShortcutPlatform;
}

export function WindowChrome({
  onImportMenuAction,
  onOpenSettingsMenuAction,
  onToggleSettings,
  onToggleSidebar,
  platform = getShortcutPlatform(),
  settingsOpen,
  sidebarVisible,
}: WindowChromeProps) {
  if (getWindowChromeVariant(platform) === "mac") {
    return (
      <Toolbar
        onToggleSidebar={onToggleSidebar}
        onToggleSettings={onToggleSettings}
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
