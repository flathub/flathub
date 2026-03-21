export function buildPlainTextPageStartIndices(
  lineHeights: number[],
  availableHeight: number,
  gap: number,
): number[] {
  if (lineHeights.length === 0) {
    return [0];
  }

  const safeAvailableHeight = Math.max(1, Math.floor(availableHeight));
  const safeGap = Math.max(0, Math.floor(gap));
  const pageStartIndices = [0];
  let currentPageStart = 0;
  let usedHeight = 0;

  for (let index = 0; index < lineHeights.length; index += 1) {
    const lineHeight = Math.max(1, Math.ceil(lineHeights[index] ?? 0));
    const nextUsedHeight =
      index === currentPageStart
        ? lineHeight
        : usedHeight + safeGap + lineHeight;

    if (index !== currentPageStart && nextUsedHeight > safeAvailableHeight) {
      currentPageStart = index;
      pageStartIndices.push(index);
      usedHeight = lineHeight;
      continue;
    }

    usedHeight = nextUsedHeight;
  }

  return pageStartIndices;
}
