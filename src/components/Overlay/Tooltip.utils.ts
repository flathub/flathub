export type TooltipVisibilityAction =
  | { type: "pointer-enter" }
  | { type: "pointer-leave" }
  | { type: "focus" }
  | { type: "blur" }
  | { type: "escape" };

interface TooltipRect {
  top: number;
  left: number;
  width: number;
  height: number;
  right: number;
  bottom: number;
}

interface TooltipSize {
  width: number;
  height: number;
}

interface TooltipViewport {
  width: number;
  height: number;
}

const TOOLTIP_GAP_PX = 8;
const TOOLTIP_VIEWPORT_PADDING_PX = 8;

export function tooltipVisibilityReducer(
  _state: boolean,
  action: TooltipVisibilityAction,
): boolean {
  switch (action.type) {
    case "pointer-enter":
    case "focus":
      return true;
    case "pointer-leave":
    case "blur":
    case "escape":
      return false;
  }
}

export function getTooltipPosition(
  anchorRect: TooltipRect,
  tooltipSize: TooltipSize,
  viewport: TooltipViewport,
): { left: number; top: number } {
  const unclampedLeft =
    anchorRect.left + anchorRect.width / 2 - tooltipSize.width / 2;
  const left = Math.min(
    Math.max(unclampedLeft, TOOLTIP_VIEWPORT_PADDING_PX),
    viewport.width - tooltipSize.width - TOOLTIP_VIEWPORT_PADDING_PX,
  );

  const topAbove = anchorRect.top - tooltipSize.height - TOOLTIP_GAP_PX;
  const top =
    topAbove >= TOOLTIP_VIEWPORT_PADDING_PX
      ? topAbove
      : Math.min(
          anchorRect.bottom + TOOLTIP_GAP_PX,
          viewport.height - tooltipSize.height - TOOLTIP_VIEWPORT_PADDING_PX,
        );

  return { left, top };
}
