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
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar,
      adjustLyricsFont: vi.fn(),
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar,
      adjustLyricsFont: vi.fn(),
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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

  test("increases lyric font size with the shared primary shortcut", () => {
    const adjustLyricsFont = vi.fn();
    const event = createKeyboardEvent({
      code: "Equal",
      key: "+",
      metaKey: true,
      shiftKey: true,
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont,
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
    expect(adjustLyricsFont).toHaveBeenCalledWith(1);
  });

  test("decreases lyric font size with the shared primary shortcut", () => {
    const adjustLyricsFont = vi.fn();
    const event = createKeyboardEvent({
      code: "Minus",
      key: "-",
      ctrlKey: true,
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont,
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
    expect(adjustLyricsFont).toHaveBeenCalledWith(-1);
  });

  test("resets lyric font size with the shared primary shortcut", () => {
    const resetLyricsFont = vi.fn();
    const event = createKeyboardEvent({
      code: "Digit0",
      key: "0",
      metaKey: true,
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont: vi.fn(),
      resetLyricsFont,
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
    expect(resetLyricsFont).toHaveBeenCalledOnce();
  });

  test("ignores lyric font shortcuts while typing in an input", () => {
    const adjustLyricsFont = vi.fn();
    const event = createKeyboardEvent({
      code: "Equal",
      key: "+",
      ctrlKey: true,
      shiftKey: true,
      target: createKeyboardTarget("TEXTAREA"),
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont,
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
    expect(adjustLyricsFont).not.toHaveBeenCalled();
  });

  test("opens the import dialog with the shared primary shortcut", () => {
    const openImportDialog = vi.fn();
    const event = createKeyboardEvent({
      code: "KeyO",
      key: "o",
      metaKey: true,
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog,
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont: vi.fn(),
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
    expect(openImportDialog).toHaveBeenCalledOnce();
  });

  test("ignores the import shortcut while typing in an input", () => {
    const openImportDialog = vi.fn();
    const event = createKeyboardEvent({
      code: "KeyO",
      key: "o",
      ctrlKey: true,
      target: createKeyboardTarget("INPUT"),
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog,
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont: vi.fn(),
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage: vi.fn(),
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
    expect(openImportDialog).not.toHaveBeenCalled();
  });

  test("steps the remote plain-text lyrics page backward with PageUp", () => {
    const stepPlainTextPage = vi.fn();
    const event = createKeyboardEvent({
      code: "PageUp",
      key: "PageUp",
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont: vi.fn(),
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: true,
      stepPlainTextPage,
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
    expect(stepPlainTextPage).toHaveBeenCalledWith("prev");
  });

  test("ignores PageDown when plain-text remote paging is unavailable", () => {
    const stepPlainTextPage = vi.fn();
    const event = createKeyboardEvent({
      code: "PageDown",
      key: "PageDown",
    });

    const handled = handleAppKeyDown(event, {
      openImportDialog: vi.fn(),
      toggleSettings: vi.fn(),
      toggleSidebar: vi.fn(),
      adjustLyricsFont: vi.fn(),
      resetLyricsFont: vi.fn(),
      canStepPlainTextPage: false,
      stepPlainTextPage,
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
    expect(stepPlainTextPage).not.toHaveBeenCalled();
  });
});
