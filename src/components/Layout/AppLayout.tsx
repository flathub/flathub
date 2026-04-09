import { useCallback } from "react";
import { Sidebar } from "./Sidebar";
import { SidebarRail } from "./SidebarRail";
import { WindowChrome } from "./WindowChrome";
import { ToastContainer } from "./ToastContainer";
import { MainContentView } from "./MainContentView";
import { ImportCdgChoiceDialog } from "@/components/Library/ImportCdgChoiceDialog";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import {
  createWindowShellStyle,
  type WindowShellState,
  useWindowShellState,
} from "@/lib/window-shell";
import { promptImportFiles } from "@/runtime/menu-runtime";
import { useLayoutStore } from "@/stores/layout-store";
import { useLibraryStore } from "@/stores/library-store";
import { useSettingsStore } from "@/stores/settings-store";

interface AppLayoutProps {
  initialWindowShellState?: WindowShellState;
}

export function AppLayout({ initialWindowShellState }: AppLayoutProps) {
  const sidebarVisible = useLayoutStore((s) => s.sidebarVisible);
  const sidebarWidth = useLayoutStore((s) => s.sidebarWidth);
  const setSidebarWidth = useLayoutStore((s) => s.setSidebarWidth);
  const toggleSidebar = useLayoutStore((s) => s.toggleSidebar);
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const openSettings = useSettingsStore((s) => s.open);
  const toggleSettings = useSettingsStore((s) => s.toggle);
  const importFiles = useLibraryStore((s) => s.importFiles);
  const platform = getShortcutPlatform();
  const windowShellState = useWindowShellState(
    initialWindowShellState,
    platform,
  );

  const handleImportMenuAction = useCallback(() => {
    return promptImportFiles({ importFiles });
  }, [importFiles]);

  return (
    <div
      className="flex h-screen w-full flex-col overflow-hidden font-sans"
      data-window-chrome-platform={windowShellState.chromeVariant}
      data-window-shell-tier={windowShellState.tier}
      style={createWindowShellStyle({
        ...windowShellState,
        sidebarWidth,
      })}
    >
      <WindowChrome
        onImportMenuAction={handleImportMenuAction}
        onOpenSettingsMenuAction={openSettings}
        onToggleSidebar={toggleSidebar}
        onToggleSettings={toggleSettings}
        shellState={windowShellState}
        settingsOpen={settingsOpen}
        sidebarVisible={sidebarVisible}
      />

      <div className="flex min-h-0 flex-1 overflow-hidden">
        <SidebarRail
          visible={sidebarVisible}
          width={sidebarWidth}
          onResize={setSidebarWidth}
        >
          <Sidebar />
        </SidebarRail>

        <MainContentView />
      </div>

      <ToastContainer />
      <ImportCdgChoiceDialog />
    </div>
  );
}
