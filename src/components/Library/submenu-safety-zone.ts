export type Point = { x: number; y: number };

export function pointInConvexPolygon(point: Point, vertices: Point[]): boolean {
  const n = vertices.length;
  if (n < 3) return false;

  let sign = 0;
  for (let i = 0; i < n; i++) {
    const a = vertices[i];
    const b = vertices[(i + 1) % n];
    const cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
    const s = Math.sign(cross);
    if (s !== 0 && sign === 0) sign = s;
    if (s !== 0 && s !== sign) return false;
  }
  return true;
}

export function isInSafetyZone(
  mouse: Point,
  parentRect: DOMRect,
  submenuRect: DOMRect,
): boolean {
  const vertices: Point[] = [
    { x: parentRect.right, y: parentRect.top },
    { x: parentRect.right, y: parentRect.bottom },
    { x: submenuRect.left, y: submenuRect.bottom },
    { x: submenuRect.left, y: submenuRect.top },
  ];
  return pointInConvexPolygon(mouse, vertices);
}
