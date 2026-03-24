import { useCallback } from "react";
import { Sidebar } from "./Sidebar";
import { SidebarRail } from "./SidebarRail";
import { MacMainUtilityPill } from "./MacMainUtilityPill";
import { WindowChrome } from "./WindowChrome";
import { ToastContainer } from "./ToastContainer";
import { PlaybackBar } from "@/components/Player/PlaybackBar";
import { GlobalProgressBar } from "@/components/Layout/GlobalProgressBar";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { SettingsOverlay } from "@/components/Settings/SettingsOverlay";
import { ModelBootstrapBanner } from "@/components/Bootstrap/ModelBootstrapBanner";
import { QueuePanel } from "@/components/Player/QueuePanel";
import { ImportCdgChoiceDialog } from "@/components/Library/ImportCdgChoiceDialog";
import { useAnimatedPresence } from "@/hooks/use-animated-presence";
import { getWindowChromeVariant } from "@/lib/window-chrome";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { promptImportFiles } from "@/runtime/menu-runtime";
import { useSettingsStore } from "@/stores/settings-store";
import { useLayoutStore } from "@/stores/layout-store";
import { useQueueStore } from "@/stores/queue-store";
import { useLibraryStore } from "@/stores/library-store";

export function AppLayout() {
  const sidebarVisible = useLayoutStore((s) => s.sidebarVisible);
  const toggleSidebar = useLayoutStore((s) => s.toggleSidebar);
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const openSettings = useSettingsStore((s) => s.open);
  const queueOpen = useQueueStore((s) => s.isOpen);
  const toggleSettings = useSettingsStore((s) => s.toggle);
  const importFiles = useLibraryStore((s) => s.importFiles);
  const windowChromeVariant = getWindowChromeVariant(getShortcutPlatform());

  const handleImportMenuAction = useCallback(() => {
    return promptImportFiles({ importFiles });
  }, [importFiles]);

  const queueSidebar = useAnimatedPresence(
    queueOpen,
    "animate-slide-in-right",
    "animate-slide-out-right",
  );

  return (
    <div
      className="flex h-screen w-full flex-col overflow-hidden font-sans"
      data-window-chrome-platform={windowChromeVariant}
    >
      <WindowChrome
        onImportMenuAction={handleImportMenuAction}
        onOpenSettingsMenuAction={openSettings}
        onToggleSidebar={toggleSidebar}
        onToggleSettings={toggleSettings}
        settingsOpen={settingsOpen}
        sidebarVisible={sidebarVisible}
      />

      <div className="flex min-h-0 flex-1 overflow-hidden">
        <SidebarRail visible={sidebarVisible}>
          <Sidebar
            onToggleSidebar={toggleSidebar}
            sidebarVisible={sidebarVisible}
            integratedWindowHeader={windowChromeVariant === "mac"}
          />
        </SidebarRail>

        <div
          className={`flex min-w-0 flex-1 flex-col ${settingsOpen ? "bg-[var(--color-surface-muted)]" : "bg-[var(--color-surface)]"}`}
        >
          <div className="relative flex min-h-0 flex-1 overflow-hidden">
            {windowChromeVariant === "mac" && !settingsOpen ? (
              <div
                className="absolute inset-0 z-0"
                data-tauri-drag-region
                aria-hidden
              />
            ) : null}
            {settingsOpen ? (
              <SettingsOverlay />
            ) : (
              <>
                <div
                  className={`relative z-10 flex min-h-0 min-w-0 flex-1 overflow-hidden ${windowChromeVariant === "mac" ? "pointer-events-none" : ""}`}
                >
                  <div
                    className={`flex min-w-0 flex-1 flex-col overflow-hidden ${windowChromeVariant === "mac" ? "pointer-events-none" : ""}`}
                  >
                    <div
                      className={
                        windowChromeVariant === "mac"
                          ? "pointer-events-auto shrink-0"
                          : undefined
                      }
                    >
                      <ModelBootstrapBanner />
                    </div>
                    <PlaybackStage />
                  </div>
                  {queueSidebar.shouldRender && (
                    <div
                      className={`pointer-events-auto h-full ${queueSidebar.className}`}
                      onAnimationEnd={queueSidebar.onAnimationEnd}
                    >
                      <QueuePanel />
                    </div>
                  )}
                </div>
                {windowChromeVariant === "mac" ? (
                  <MacMainUtilityPill
                    onToggleSettings={toggleSettings}
                    settingsOpen={settingsOpen}
                  />
                ) : null}
              </>
            )}
          </div>

          <GlobalProgressBar />
          <PlaybackBar />
        </div>
      </div>

      <ToastContainer />
      <ImportCdgChoiceDialog />
    </div>
  );
}
