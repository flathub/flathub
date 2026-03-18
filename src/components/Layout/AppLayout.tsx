import { Sidebar } from "./Sidebar";
import { SidebarRail } from "./SidebarRail";
import { Toolbar } from "./Toolbar";
import { ToastContainer } from "./ToastContainer";
import { PlaybackBar } from "@/components/Player/PlaybackBar";
import { GlobalProgressBar } from "@/components/Layout/GlobalProgressBar";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { SettingsOverlay } from "@/components/Settings/SettingsOverlay";
import { ModelBootstrapBanner } from "@/components/Bootstrap/ModelBootstrapBanner";
import { QueuePanel } from "@/components/Player/QueuePanel";
import { ImportCdgChoiceDialog } from "@/components/Library/ImportCdgChoiceDialog";
import { useAnimatedPresence } from "@/hooks/use-animated-presence";
import { useSettingsStore } from "@/stores/settings-store";
import { useLayoutStore } from "@/stores/layout-store";
import { useQueueStore } from "@/stores/queue-store";

export function AppLayout() {
  const sidebarVisible = useLayoutStore((s) => s.sidebarVisible);
  const toggleSidebar = useLayoutStore((s) => s.toggleSidebar);
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const queueOpen = useQueueStore((s) => s.isOpen);
  const toggleSettings = useSettingsStore((s) => s.toggle);

  const queueSidebar = useAnimatedPresence(
    queueOpen,
    "animate-slide-in-right",
    "animate-slide-out-right",
  );

  return (
    <div className="flex h-screen w-full flex-col overflow-hidden">
      <Toolbar
        onToggleSidebar={toggleSidebar}
        onToggleSettings={toggleSettings}
        settingsOpen={settingsOpen}
        sidebarVisible={sidebarVisible}
      />

      <div className="flex min-h-0 flex-1 overflow-hidden">
        <SidebarRail visible={sidebarVisible}>
          <Sidebar />
        </SidebarRail>

        <div
          className={`flex min-w-0 flex-1 flex-col ${settingsOpen ? "bg-[#1a1a1c]" : "bg-[var(--color-surface)]"}`}
        >
          <div className="relative flex min-h-0 flex-1 overflow-hidden">
            {settingsOpen ? (
              <SettingsOverlay />
            ) : (
              <>
                <div className="flex min-w-0 flex-1 flex-col overflow-hidden">
                  <ModelBootstrapBanner />
                  <PlaybackStage />
                </div>
                {queueSidebar.shouldRender && (
                  <div
                    className={`h-full ${queueSidebar.className}`}
                    onAnimationEnd={queueSidebar.onAnimationEnd}
                  >
                    <QueuePanel />
                  </div>
                )}
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
