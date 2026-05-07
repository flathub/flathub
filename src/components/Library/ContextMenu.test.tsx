import { renderToStaticMarkup } from "react-dom/server";
import { beforeEach, describe, expect, test, vi } from "vitest";

const { createPortal } = vi.hoisted(() => ({
  createPortal: vi.fn((node: unknown) => node),
}));

vi.mock("react-dom", () => ({
  createPortal,
}));

import { ContextMenu } from "./ContextMenu";
import { isInSafetyZone, pointInConvexPolygon } from "./submenu-safety-zone";
import type { Point } from "./submenu-safety-zone";
import { getContextMenuPosition } from "./context-menu-position";

describe("getContextMenuPosition", () => {
  test("keeps the menu inside the viewport", () => {
    expect(
      getContextMenuPosition({
        x: 470,
        y: 490,
        menuWidth: 180,
        menuHeight: 140,
        viewportWidth: 500,
        viewportHeight: 520,
      }),
    ).toEqual({ left: 312, top: 372 });
  });
});

describe("ContextMenu", () => {
  beforeEach(() => {
    createPortal.mockClear();
    vi.stubGlobal("document", { body: { nodeName: "BODY" } });
  });

  test("renders through a portal to document.body", () => {
    renderToStaticMarkup(
      <ContextMenu
        x={32}
        y={48}
        items={[{ label: "Menu Item", onClick: () => {} }]}
        onClose={() => {}}
      />,
    );

    expect(createPortal).toHaveBeenCalledTimes(1);
    expect(createPortal).toHaveBeenCalledWith(expect.anything(), document.body);
  });
});

describe("pointInConvexPolygon", () => {
  test("point inside a square returns true", () => {
    const square: Point[] = [
      { x: 0, y: 0 },
      { x: 10, y: 0 },
      { x: 10, y: 10 },
      { x: 0, y: 10 },
    ];
    expect(pointInConvexPolygon({ x: 5, y: 5 }, square)).toBe(true);
  });

  test("point outside a square returns false", () => {
    const square: Point[] = [
      { x: 0, y: 0 },
      { x: 10, y: 0 },
      { x: 10, y: 10 },
      { x: 0, y: 10 },
    ];
    expect(pointInConvexPolygon({ x: 15, y: 5 }, square)).toBe(false);
  });

  test("point on the edge returns true", () => {
    const square: Point[] = [
      { x: 0, y: 0 },
      { x: 10, y: 0 },
      { x: 10, y: 10 },
      { x: 0, y: 10 },
    ];
    expect(pointInConvexPolygon({ x: 10, y: 5 }, square)).toBe(true);
  });

  test("fewer than 3 vertices returns false", () => {
    expect(
      pointInConvexPolygon({ x: 0, y: 0 }, [
        { x: 0, y: 0 },
        { x: 10, y: 10 },
      ]),
    ).toBe(false);
  });

  test("triangle: point inside returns true", () => {
    const triangle: Point[] = [
      { x: 0, y: 0 },
      { x: 10, y: 0 },
      { x: 5, y: 10 },
    ];
    expect(pointInConvexPolygon({ x: 5, y: 5 }, triangle)).toBe(true);
  });
});

describe("isInSafetyZone", () => {
  const parent: DOMRect = {
    x: 200,
    y: 300,
    width: 120,
    height: 24,
    top: 300,
    right: 320,
    bottom: 324,
    left: 200,
    toJSON: () => ({}),
  };

  const submenu: DOMRect = {
    x: 330,
    y: 300,
    width: 140,
    height: 336,
    top: 300,
    right: 470,
    bottom: 636,
    left: 330,
    toJSON: () => ({}),
  };

  test("returns true when mouse is in the triangle zone between parent and submenu", () => {
    const mouse = { x: 325, y: 310 };
    expect(isInSafetyZone(mouse, parent, submenu)).toBe(true);
  });

  test("returns true when mouse is in the middle of the triangle zone", () => {
    const mouse = { x: 325, y: 400 };
    expect(isInSafetyZone(mouse, parent, submenu)).toBe(true);
  });

  test("returns false when mouse is above the safety zone", () => {
    const mouse = { x: 325, y: 280 };
    expect(isInSafetyZone(mouse, parent, submenu)).toBe(false);
  });

  test("returns false when mouse is below the safety zone", () => {
    const mouse = { x: 325, y: 650 };
    expect(isInSafetyZone(mouse, parent, submenu)).toBe(false);
  });

  test("returns false when mouse is far to the right of the submenu", () => {
    const mouse = { x: 500, y: 400 };
    expect(isInSafetyZone(mouse, parent, submenu)).toBe(false);
  });
});
