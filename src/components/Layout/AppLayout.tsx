import { useState } from "react";
import { Sidebar } from "./Sidebar";
import { Toolbar } from "./Toolbar";
import { ToastContainer } from "./ToastContainer";
import { PlaybackBar } from "@/components/Player/PlaybackBar";
import { LyricsPanel } from "@/components/Lyrics/LyricsPanel";
import { SettingsOverlay } from "@/components/Settings/SettingsOverlay";
import { ModelBootstrapBanner } from "@/components/Bootstrap/ModelBootstrapBanner";
import { QueuePanel } from "@/components/Player/QueuePanel";
import { useAnimatedPresence } from "@/hooks/use-animated-presence";
import { useSettingsStore } from "@/stores/settings-store";
import { useQueueStore } from "@/stores/queue-store";

export function AppLayout() {
  const [sidebarVisible, setSidebarVisible] = useState(true);
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const queueOpen = useQueueStore((s) => s.isOpen);
  const toggleSettings = useSettingsStore((s) => s.toggle);

  const sidebar = useAnimatedPresence(
    sidebarVisible,
    "animate-slide-in-left",
    "animate-slide-out-left",
  );

  const queueSidebar = useAnimatedPresence(
    queueOpen,
    "animate-slide-in-right",
    "animate-slide-out-right",
  );

  return (
    <div className="flex h-screen w-full overflow-hidden">
      {sidebar.shouldRender && (
        <div
          className={`h-full ${sidebar.className}`}
          onAnimationEnd={sidebar.onAnimationEnd}
        >
          <Sidebar />
        </div>
      )}

      <div
        className={`flex flex-1 flex-col ${settingsOpen ? "bg-[#1a1a1c]" : "bg-[var(--color-surface)]"}`}
      >
        <Toolbar
          onToggleSidebar={() => setSidebarVisible(!sidebarVisible)}
          onToggleSettings={toggleSettings}
          settingsOpen={settingsOpen}
          sidebarVisible={sidebarVisible}
        />

        <div className="relative flex flex-1 overflow-hidden">
          {settingsOpen ? (
            <SettingsOverlay />
          ) : (
            <>
              <div className="flex flex-1 flex-col overflow-hidden">
                <ModelBootstrapBanner />
                <LyricsPanel />
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

        <PlaybackBar />
      </div>

      <ToastContainer />
    </div>
  );
}
