import { PlaybackStage } from "@/components/Playback/PlaybackStage";
import { useCdgFrameReceiver } from "@/hooks/use-cdg-frame-receiver";
import { useLyricsSync } from "@/hooks/use-lyrics-sync";
import {
  useFullscreenPlaybackRuntime,
  useLyricsAutoFetch,
} from "@/hooks/use-playback-runtime";
import { FullscreenControls } from "./FullscreenControls";

export function FullscreenPlayerView() {
  useFullscreenPlaybackRuntime();
  useLyricsAutoFetch();
  useLyricsSync();
  useCdgFrameReceiver();

  return (
    <div className="relative flex h-screen w-screen flex-col bg-black">
      <div className="flex flex-1 overflow-hidden">
        <PlaybackStage presentation="audience" />
      </div>
      <FullscreenControls />
    </div>
  );
}
