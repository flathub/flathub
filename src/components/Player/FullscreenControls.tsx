import { useEffect, useRef } from "react";
import { PlayControls } from "./PlayControls";
import { SeekBar } from "./SeekBar";
import { useMouseIdle } from "@/hooks/use-mouse-idle";
import { closeFullscreenPlayer } from "@/lib/fullscreen-player";

interface FullscreenControlsProps {
  onHeightChange?: (height: number) => void;
}

export function FullscreenControls({
  onHeightChange,
}: FullscreenControlsProps = {}) {
  const idle = useMouseIdle(3000);
  const containerRef = useRef<HTMLDivElement>(null);

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

  useEffect(() => {
    if (!onHeightChange) {
      return;
    }

    const measure = () => {
      const height = Math.ceil(
        containerRef.current?.getBoundingClientRect().height ?? 0,
      );
      if (height > 0) {
        onHeightChange(height);
      }
    };

    measure();

    if (typeof ResizeObserver !== "undefined" && containerRef.current) {
      const observer = new ResizeObserver(measure);
      observer.observe(containerRef.current);
      return () => observer.disconnect();
    }

    window.addEventListener("resize", measure);
    return () => {
      window.removeEventListener("resize", measure);
    };
  }, [onHeightChange]);

  return (
    <div
      ref={containerRef}
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
