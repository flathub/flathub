import { useEffect } from "react";
import { useTranslation } from "react-i18next";
import { X } from "lucide-react";
import { PlayControls } from "./PlayControls";
import { SeekBar } from "./SeekBar";
import { useMouseIdle } from "@/hooks/use-mouse-idle";
import { closeFullscreenPlayer } from "@/lib/fullscreen-player";

interface FullscreenControlsProps {
  onCursorVisibilityChange?: (visible: boolean) => void;
}

export function FullscreenControls({
  onCursorVisibilityChange,
}: FullscreenControlsProps) {
  const { t } = useTranslation();
  const idle = useMouseIdle(3000);

  useEffect(() => {
    onCursorVisibilityChange?.(!idle);
  }, [idle, onCursorVisibilityChange]);

  return (
    <>
      {/* Exit fullscreen button — top right */}
      <div
        className={`absolute right-4 top-4 z-50 transition-opacity duration-300 ${
          idle ? "pointer-events-none opacity-0" : "opacity-100"
        }`}
      >
        <button
          onClick={() => closeFullscreenPlayer()}
          className="rounded-full bg-black/60 p-2 text-white/80 backdrop-blur-sm transition-colors hover:bg-black/80 hover:text-white"
          title={t("player.exitFullscreen")}
          aria-label={t("player.exitFullscreen")}
        >
          <X size={20} />
        </button>
      </div>

      {/* Bottom playback bar overlay */}
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
    </>
  );
}
