import { QueuePanel } from "@/components/Player/QueuePanel";
import { GlobalProgressBar } from "@/components/Layout/GlobalProgressBar";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { SettingsOverlay } from "@/components/Settings/SettingsOverlay";
import { ModelBootstrapBanner } from "@/components/Bootstrap/ModelBootstrapBanner";
import { PlaybackBar } from "@/components/Player/PlaybackBar";
import { NativeFloatingControls } from "@/components/Layout/NativeFloatingControls";
import { useAnimatedPresence } from "@/hooks/use-animated-presence";
import { useSettingsStore } from "@/stores/settings-store";
import { useQueueStore } from "@/stores/queue-store";
import type { WindowShellTier } from "@/types/ipc";

interface MainContentViewProps {
  shellTier?: WindowShellTier;
}

export function MainContentView({
  shellTier = "desktop",
}: MainContentViewProps) {
  const settingsOpen = useSettingsStore((s) => s.isOpen);
  const queueOpen = useQueueStore((s) => s.isOpen);
  const nativeVariant = shellTier === "mac_native";
  const queueSidebar = useAnimatedPresence(
    queueOpen,
    "animate-slide-in-right",
    "animate-slide-out-right",
  );

  return (
    <div
      className={`flex min-w-0 flex-1 flex-col ${settingsOpen ? "bg-[var(--color-surface-muted)]" : "bg-[var(--color-surface)]"}`}
      data-main-content-visual-variant={nativeVariant ? "native" : "default"}
    >
      <div className="relative flex min-h-0 flex-1 overflow-hidden">
        {nativeVariant ? <NativeFloatingControls /> : null}
        {settingsOpen ? (
          <SettingsOverlay />
        ) : (
          <>
            <div className="flex min-w-0 flex-1 flex-col overflow-hidden">
              <ModelBootstrapBanner />
              <PlaybackStage shellTier={shellTier} />
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
      <PlaybackBar shellTier={shellTier} />
    </div>
  );
}
