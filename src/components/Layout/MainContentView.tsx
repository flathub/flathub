import { QueuePanel } from "@/components/Player/QueuePanel";
import { GlobalProgressBar } from "@/components/Layout/GlobalProgressBar";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { SettingsOverlay } from "@/components/Settings/SettingsOverlay";
import { ModelBootstrapBanner } from "@/components/Bootstrap/ModelBootstrapBanner";
import { PlaybackBar } from "@/components/Player/PlaybackBar";
import { useAnimatedPresence } from "@/hooks/use-animated-presence";
import { useSettingsStore } from "@/stores/settings-store";
import { useQueueStore } from "@/stores/queue-store";

export function MainContentView() {
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const queueOpen = useQueueStore((s) => s.isOpen);
  const queueSidebar = useAnimatedPresence(
    queueOpen,
    "animate-slide-in-right",
    "animate-slide-out-right",
  );

  return (
    <div
      className={`flex min-w-0 flex-1 flex-col ${settingsOpen ? "bg-[var(--color-surface-muted)]" : "bg-[var(--color-surface)]"}`}
      data-main-content-visual-variant="unified"
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
  );
}
