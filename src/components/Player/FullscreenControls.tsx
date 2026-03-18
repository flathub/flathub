import { useEffect } from "react";
import { PlayControls } from "./PlayControls";
import { SeekBar } from "./SeekBar";
import { useMouseIdle } from "@/hooks/use-mouse-idle";
import { closeFullscreenPlayer } from "@/lib/fullscreen-player";

export function FullscreenControls() {
  const idle = useMouseIdle(3000);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        void closeFullscreenPlayer();
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => {
      window.removeEventListener("keydown", handleKeyDown);
    };
  }, []);

  return (
    <div
      className={`absolute inset-x-0 bottom-0 z-50 bg-gradient-to-t from-black/80 to-transparent px-8 pb-6 pt-16 transition-opacity duration-300 ${
        idle ? "pointer-events-none opacity-0" : "opacity-100"
      }`}
    >
      <div className="flex items-center justify-center gap-6">
        <PlayControls />
        <div className="w-full max-w-2xl">
          <SeekBar />
        </div>
      </div>
    </div>
  );
}
