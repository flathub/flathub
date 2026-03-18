import { useState, useCallback } from "react";
import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { useCdgFrameReceiver } from "@/hooks/use-cdg-frame-receiver";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import {
  useFullscreenPlaybackRuntime,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
import { FullscreenControls } from "./FullscreenControls";

export function FullscreenPlayerView() {
  const [cursorVisible, setCursorVisible] = useState(true);

  useFullscreenPlaybackRuntime();
  useLyricsAutoFetch();
  useLyricsSync();
  useCdgFrameReceiver();

  const handleCursorVisibility = useCallback((visible: boolean) => {
    setCursorVisible(visible);
  }, []);

  return (
    <div
      className="relative flex h-screen w-screen flex-col bg-black"
      style={{ cursor: cursorVisible ? "default" : "none" }}
    >
      <div className="flex flex-1 overflow-hidden">
        <PlaybackStage presentation="audience" />
      </div>
      <FullscreenControls onCursorVisibilityChange={handleCursorVisibility} />
    </div>
  );
}
