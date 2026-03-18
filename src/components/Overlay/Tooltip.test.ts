import { describe, expect, test } from "vitest";
import { getTooltipPosition, tooltipVisibilityReducer } from "./Tooltip.utils";

describe("tooltipVisibilityReducer", () => {
  test("opens immediately on pointer and focus interactions", () => {
    expect(tooltipVisibilityReducer(false, { type: "pointer-enter" })).toBe(
      true,
    );
    expect(tooltipVisibilityReducer(false, { type: "focus" })).toBe(true);
  });

  test("closes on pointer leave, blur, and escape", () => {
    expect(tooltipVisibilityReducer(true, { type: "pointer-leave" })).toBe(
      false,
    );
    expect(tooltipVisibilityReducer(true, { type: "blur" })).toBe(false);
    expect(tooltipVisibilityReducer(true, { type: "escape" })).toBe(false);
  });
});

describe("getTooltipPosition", () => {
  test("centers above the trigger when space is available", () => {
    expect(
      getTooltipPosition(
        {
          top: 80,
          left: 100,
          width: 32,
          height: 32,
          bottom: 112,
          right: 132,
        },
        { width: 120, height: 40 },
        { width: 400, height: 300 },
      ),
    ).toEqual({ left: 56, top: 32 });
  });

  test("falls back below the trigger and clamps to the viewport", () => {
    expect(
      getTooltipPosition(
        {
          top: 8,
          left: 4,
          width: 32,
          height: 32,
          bottom: 40,
          right: 36,
        },
        { width: 180, height: 40 },
        { width: 200, height: 120 },
      ),
    ).toEqual({ left: 8, top: 48 });
  });
});
