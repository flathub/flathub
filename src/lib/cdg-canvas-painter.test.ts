import { beforeEach, describe, expect, test, vi } from "vitest";
import {
  clearFrame,
  drawFrame,
  setCdgCanvas,
  CDG_HEIGHT,
  CDG_WIDTH,
} from "./cdg-canvas-painter";

class MockImageData {
  data: Uint8ClampedArray;
  width: number;
  height: number;

  constructor(width: number, height: number) {
    this.width = width;
    this.height = height;
    this.data = new Uint8ClampedArray(width * height * 4);
  }
}

describe("cdg canvas painter", () => {
  beforeEach(() => {
    vi.stubGlobal("ImageData", MockImageData);
    clearFrame();
    setCdgCanvas(null);
  });

  test("replays the latest frame after the canvas mounts late", () => {
    const rgba = new Uint8Array(CDG_WIDTH * CDG_HEIGHT * 4);
    rgba[0] = 17;
    rgba[1] = 34;
    rgba[2] = 51;
    rgba[3] = 255;

    drawFrame(rgba.buffer);

    const ctx = {
      putImageData: vi.fn(),
      clearRect: vi.fn(),
    } as unknown as CanvasRenderingContext2D;
    const canvas = {
      getContext: vi.fn(() => ctx),
    } as unknown as HTMLCanvasElement;

    setCdgCanvas(canvas);

    expect(ctx.putImageData).toHaveBeenCalledOnce();
    const imageData = vi.mocked(ctx.putImageData).mock
      .calls[0]?.[0] as MockImageData;
    expect(imageData.data[0]).toBe(17);
    expect(imageData.data[1]).toBe(34);
    expect(imageData.data[2]).toBe(51);
    expect(imageData.data[3]).toBe(255);
  });
});
