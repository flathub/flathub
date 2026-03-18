import { describe, expect, test, vi } from "vitest";
import { handleAppKeyDown } from "./use-keyboard-shortcuts";

function createKeyboardTarget(
  tagName: string,
  isContentEditable = false,
): EventTarget & { tagName: string; isContentEditable: boolean } {
  return { tagName, isContentEditable } as EventTarget & {
    tagName: string;
    isContentEditable: boolean;
  };
}

function createKeyboardEvent(
  overrides: Partial<KeyboardEvent> & Pick<KeyboardEvent, "code" | "key">,
): KeyboardEvent {
  return {
    altKey: false,
    ctrlKey: false,
    metaKey: false,
    shiftKey: false,
    preventDefault: vi.fn(),
    target: createKeyboardTarget("DIV"),
    ...overrides,
  } as KeyboardEvent;
}

describe("handleAppKeyDown", () => {
  test("toggles the sidebar with the shared primary shortcut", () => {
    const toggleSidebar = vi.fn();
    const event = createKeyboardEvent({
      code: "KeyB",
      key: "b",
      metaKey: true,
    });

    const handled = handleAppKeyDown(event, {
      toggleSettings: vi.fn(),
      toggleSidebar,
      player: {
        snapshot: null,
        positionMs: 0,
        pause: vi.fn(),
        resume: vi.fn(),
        seek: vi.fn(),
        setVolume: vi.fn(),
      },
    });

    expect(handled).toBe(true);
    expect(event.preventDefault).toHaveBeenCalledOnce();
    expect(toggleSidebar).toHaveBeenCalledOnce();
  });

  test("ignores the sidebar shortcut while typing in an input", () => {
    const toggleSidebar = vi.fn();
    const event = createKeyboardEvent({
      code: "KeyB",
      key: "b",
      ctrlKey: true,
      target: createKeyboardTarget("INPUT"),
    });

    const handled = handleAppKeyDown(event, {
      toggleSettings: vi.fn(),
      toggleSidebar,
      player: {
        snapshot: null,
        positionMs: 0,
        pause: vi.fn(),
        resume: vi.fn(),
        seek: vi.fn(),
        setVolume: vi.fn(),
      },
    });

    expect(handled).toBe(false);
    expect(toggleSidebar).not.toHaveBeenCalled();
  });
});
