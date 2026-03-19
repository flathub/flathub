interface ContextMenuPositionArgs {
  x: number;
  y: number;
  menuWidth: number;
  menuHeight: number;
  viewportWidth: number;
  viewportHeight: number;
}

const VIEWPORT_PADDING = 8;

export function getContextMenuPosition({
  x,
  y,
  menuWidth,
  menuHeight,
  viewportWidth,
  viewportHeight,
}: ContextMenuPositionArgs) {
  const maxLeft = Math.max(
    VIEWPORT_PADDING,
    viewportWidth - menuWidth - VIEWPORT_PADDING,
  );
  const maxTop = Math.max(
    VIEWPORT_PADDING,
    viewportHeight - menuHeight - VIEWPORT_PADDING,
  );

  return {
    left: Math.min(Math.max(x, VIEWPORT_PADDING), maxLeft),
    top: Math.min(Math.max(y, VIEWPORT_PADDING), maxTop),
  };
}
