interface CenteredScrollTopInput {
  viewportHeight: number;
  scrollHeight: number;
  lineOffsetTop: number;
  lineHeight: number;
}

export function getCenteredScrollTop({
  viewportHeight,
  scrollHeight,
  lineOffsetTop,
  lineHeight,
}: CenteredScrollTopInput): number {
  const centeredTop = lineOffsetTop + lineHeight / 2 - viewportHeight / 2;
  const maxScrollTop = Math.max(0, scrollHeight - viewportHeight);

  return Math.max(0, Math.min(maxScrollTop, Math.round(centeredTop)));
}
