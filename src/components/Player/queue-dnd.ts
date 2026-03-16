export type DropIndicatorPosition = "above" | "below";
export type DropAnnouncementPosition = "before" | "after";

export function getDropIndicatorPosition(
  activeIndex: number | null,
  overIndex: number | null,
): DropIndicatorPosition | null {
  if (
    activeIndex === null ||
    overIndex === null ||
    activeIndex < 0 ||
    overIndex < 0 ||
    activeIndex === overIndex
  ) {
    return null;
  }

  return activeIndex > overIndex ? "above" : "below";
}

export function getDropAnnouncementPosition(
  activeIndex: number | null,
  overIndex: number | null,
): DropAnnouncementPosition | null {
  const indicator = getDropIndicatorPosition(activeIndex, overIndex);

  if (!indicator) {
    return null;
  }

  return indicator === "above" ? "before" : "after";
}
