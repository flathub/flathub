import { describe, expect, test } from "vitest";
import {
  APP_SHORTCUTS,
  getShortcutDisplay,
  matchesShortcut,
} from "./app-shortcuts";

describe("app-shortcuts", () => {
  test("formats primary shortcuts for macOS", () => {
    expect(getShortcutDisplay(APP_SHORTCUTS.toggleSidebar, "mac")).toBe("⌘B");
    expect(getShortcutDisplay(APP_SHORTCUTS.toggleSettings, "mac")).toBe("⌘,");
  });

  test("formats primary shortcuts for non-mac platforms", () => {
    expect(getShortcutDisplay(APP_SHORTCUTS.toggleSidebar, "windows")).toBe(
      "Ctrl+B",
    );
    expect(getShortcutDisplay(APP_SHORTCUTS.toggleSettings, "linux")).toBe(
      "Ctrl+,",
    );
  });

  test("matches shortcuts through the primary modifier", () => {
    expect(
      matchesShortcut(APP_SHORTCUTS.toggleSidebar, {
        code: "KeyB",
        key: "b",
        metaKey: true,
        ctrlKey: false,
        altKey: false,
        shiftKey: false,
      }),
    ).toBe(true);

    expect(
      matchesShortcut(APP_SHORTCUTS.toggleSidebar, {
        code: "KeyB",
        key: "b",
        metaKey: false,
        ctrlKey: true,
        altKey: false,
        shiftKey: false,
      }),
    ).toBe(true);

    expect(
      matchesShortcut(APP_SHORTCUTS.toggleSettings, {
        code: "Comma",
        key: ",",
        metaKey: false,
        ctrlKey: false,
        altKey: false,
        shiftKey: false,
      }),
    ).toBe(false);
  });
});
