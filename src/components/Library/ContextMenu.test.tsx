import { renderToStaticMarkup } from "react-dom/server";
import { beforeEach, describe, expect, test, vi } from "vitest";

const { createPortal } = vi.hoisted(() => ({
  createPortal: vi.fn((node: unknown) => node),
}));

vi.mock("react-dom", () => ({
  createPortal,
}));

import { ContextMenu } from "./ContextMenu";
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
