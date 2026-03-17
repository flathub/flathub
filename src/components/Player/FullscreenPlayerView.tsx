import { useState, useCallback } from "react";
import { LyricsPanel } from "@/components/Lyrics/LyricsPanel";
import { CdgCanvas } from "@/components/Cdg/CdgCanvas";
import { useCdgSync } from "@/hooks/use-cdg-sync";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import {
  useFullscreenPlaybackRuntime,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
import { FullscreenControls } from "./FullscreenControls";
import { useCdgStore } from "@/stores/cdg-store";

export function FullscreenPlayerView() {
  const hasCdg = useCdgStore((s) => s.hasCdg);
  const [cursorVisible, setCursorVisible] = useState(true);

  useFullscreenPlaybackRuntime();
  useLyricsAutoFetch();
  useLyricsSync();
  useCdgSync();

  const handleCursorVisibility = useCallback((visible: boolean) => {
    setCursorVisible(visible);
  }, []);

  return (
    <div
      className="relative h-screen w-screen bg-black"
      style={{ cursor: cursorVisible ? "default" : "none" }}
    >
      <div className="h-full w-full">
        {hasCdg ? <CdgCanvas /> : <LyricsPanel />}
      </div>
      <FullscreenControls onCursorVisibilityChange={handleCursorVisibility} />
    </div>
  );
}
