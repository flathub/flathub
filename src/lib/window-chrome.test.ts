import { describe, expect, test, vi } from "vitest";
import {
  createDesktopWindowActions,
  getWindowChromeVariant,
  syncWindowMaximizedState,
  type DesktopWindowController,
} from "./window-chrome";

function createController(): DesktopWindowController {
  return {
    close: vi.fn().mockResolvedValue(undefined),
    isMaximized: vi.fn().mockResolvedValue(false),
    minimize: vi.fn().mockResolvedValue(undefined),
    onResized: vi.fn().mockResolvedValue(() => {}),
    startDragging: vi.fn().mockResolvedValue(undefined),
    startResizeDragging: vi.fn().mockResolvedValue(undefined),
    toggleMaximize: vi.fn().mockResolvedValue(undefined),
  };
}

describe("window chrome helpers", () => {
  test("uses the mac chrome only on macOS", () => {
    expect(getWindowChromeVariant("mac")).toBe("mac");
    expect(getWindowChromeVariant("windows")).toBe("desktop");
    expect(getWindowChromeVariant("linux")).toBe("desktop");
  });

  test("desktop window actions forward the window control commands", async () => {
    const controller = createController();
    const actions = createDesktopWindowActions(controller);

    await actions.minimize();
    await actions.toggleMaximize();
    await actions.close();
    await actions.startResizeDragging("NorthEast");

    expect(controller.minimize).toHaveBeenCalledOnce();
    expect(controller.toggleMaximize).toHaveBeenCalledOnce();
    expect(controller.close).toHaveBeenCalledOnce();
    expect(controller.startResizeDragging).toHaveBeenCalledWith("NorthEast");
  });

  test("starts dragging on single-click and toggles maximize on double-click", async () => {
    const controller = createController();
    const actions = createDesktopWindowActions(controller);

    await actions.handleDragRegionPointerDown({ buttons: 1, detail: 1 });
    await actions.handleDragRegionPointerDown({ buttons: 1, detail: 2 });

    expect(controller.startDragging).toHaveBeenCalledOnce();
    expect(controller.toggleMaximize).toHaveBeenCalledOnce();
  });

  test("ignores non-primary drag gestures", async () => {
    const controller = createController();
    const actions = createDesktopWindowActions(controller);

    await actions.handleDragRegionPointerDown({ buttons: 2, detail: 1 });

    expect(controller.startDragging).not.toHaveBeenCalled();
    expect(controller.toggleMaximize).not.toHaveBeenCalled();
  });

  test("reads and forwards the maximized state", async () => {
    const controller = createController();
    const setMaximized = vi.fn();
    vi.mocked(controller.isMaximized).mockResolvedValue(true);

    await syncWindowMaximizedState(controller, setMaximized);

    expect(controller.isMaximized).toHaveBeenCalledOnce();
    expect(setMaximized).toHaveBeenCalledWith(true);
  });
});
