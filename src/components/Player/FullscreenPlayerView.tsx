import { useCallback, useState } from "react";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { useCdgFrameReceiver } from "@/hooks/use-cdg-frame-receiver";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import {
  useFullscreenPlaybackRuntime,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
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
