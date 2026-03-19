import { getCurrentWindow } from "@tauri-apps/api/window";
import type { ShortcutPlatform } from "@/lib/app-shortcuts";

export type WindowChromeVariant = "mac" | "desktop";
export type ResizeDirection =
  | "East"
  | "North"
  | "NorthEast"
  | "NorthWest"
  | "South"
  | "SouthEast"
  | "SouthWest"
  | "West";

export interface DragRegionPointerLike {
  buttons: number;
  detail: number;
}

export interface DesktopWindowController {
  close: () => Promise<void>;
  isMaximized: () => Promise<boolean>;
  minimize: () => Promise<void>;
  onResized: (handler: () => void) => Promise<() => void>;
  startDragging: () => Promise<void>;
  startResizeDragging: (direction: ResizeDirection) => Promise<void>;
  toggleMaximize: () => Promise<void>;
}

export function getWindowChromeVariant(
  platform: ShortcutPlatform,
): WindowChromeVariant {
  return platform === "mac" ? "mac" : "desktop";
}

export function createCurrentDesktopWindowController(): DesktopWindowController {
  const window = getCurrentWindow();

  return {
    close: () => window.close(),
    isMaximized: () => window.isMaximized(),
    minimize: () => window.minimize(),
    onResized: (handler) => window.onResized(() => handler()),
    startDragging: () => window.startDragging(),
    startResizeDragging: (direction) => window.startResizeDragging(direction),
    toggleMaximize: () => window.toggleMaximize(),
  };
}

export function createNoopDesktopWindowController(): DesktopWindowController {
  return {
    close: async () => {},
    isMaximized: async () => false,
    minimize: async () => {},
    onResized: async () => () => {},
    startDragging: async () => {},
    startResizeDragging: async () => {},
    toggleMaximize: async () => {},
  };
}

export async function syncWindowMaximizedState(
  controller: Pick<DesktopWindowController, "isMaximized">,
  setMaximized: (value: boolean) => void,
): Promise<void> {
  setMaximized(await controller.isMaximized());
}

export function createDesktopWindowActions(
  controller: Pick<
    DesktopWindowController,
    | "close"
    | "minimize"
    | "startDragging"
    | "startResizeDragging"
    | "toggleMaximize"
  >,
) {
  return {
    close: () => controller.close(),
    handleDragRegionPointerDown: (event: DragRegionPointerLike) => {
      if (event.buttons !== 1) {
        return Promise.resolve();
      }

      if (event.detail === 2) {
        return controller.toggleMaximize();
      }

      return controller.startDragging();
    },
    minimize: () => controller.minimize(),
    startResizeDragging: (direction: ResizeDirection) =>
      controller.startResizeDragging(direction),
    toggleMaximize: () => controller.toggleMaximize(),
  };
}
