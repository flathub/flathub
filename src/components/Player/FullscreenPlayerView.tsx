import { useCallback, useEffect, useState } from "react";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { useCdgFrameReceiver } from "@/hooks/use-cdg-frame-receiver";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import {
  useFullscreenPlaybackRuntime,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
import { announceLocalAudienceOutputActive } from "@/lib/plain-text-page-controls";
import { FullscreenControls } from "./FullscreenControls";

const FULLSCREEN_STAGE_BOTTOM_INSET_PX = 144;

export function FullscreenPlayerView() {
  // Keep a conservative default so the first paint never renders beneath the
  // floating footer before the client measures its actual height.
  const [bottomInsetPx, setBottomInsetPx] = useState(
    FULLSCREEN_STAGE_BOTTOM_INSET_PX,
  );

  useFullscreenPlaybackRuntime();
  useLyricsAutoFetch();
  useLyricsSync();
  useCdgFrameReceiver();

  useEffect(() => {
    void announceLocalAudienceOutputActive(true).catch(() => {
      // The main window treats this as auxiliary state; a missed update must
      // not block opening the audience window itself.
    });

    return () => {
      void announceLocalAudienceOutputActive(false).catch(() => {
        // Closing the window should stay best-effort even if the state sync is gone.
      });
    };
  }, []);

  const handleControlsHeightChange = useCallback((height: number) => {
    const nextHeight = Math.max(
      FULLSCREEN_STAGE_BOTTOM_INSET_PX,
      Math.ceil(height),
    );
    setBottomInsetPx((current) =>
      current === nextHeight ? current : nextHeight,
    );
  }, []);

  return (
    <div className="relative flex h-screen w-screen flex-col bg-black">
      <div className="flex flex-1 overflow-hidden">
        <PlaybackStage presentation="audience" bottomInsetPx={bottomInsetPx} />
      </div>
      <FullscreenControls onHeightChange={handleControlsHeightChange} />
    </div>
  );
}
