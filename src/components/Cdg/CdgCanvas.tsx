import { useCallback } from "react";
import { setCdgCanvas } from "@/lib/cdg-canvas-painter";

const CDG_WIDTH = 288;
const CDG_HEIGHT = 192;

export function CdgCanvas() {
  const canvasCallback = useCallback((node: HTMLCanvasElement | null) => {
    setCdgCanvas(node);
  }, []);

  return (
    <div className="flex flex-1 items-center justify-center bg-black">
      <canvas
        ref={canvasCallback}
        width={CDG_WIDTH}
        height={CDG_HEIGHT}
        className="max-h-full max-w-full"
        style={{ imageRendering: "pixelated" }}
      />
    </div>
  );
}
