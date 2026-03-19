export type ShortcutPlatform = "mac" | "windows" | "linux";

export interface ShortcutEventLike {
  code: string;
  key: string;
  metaKey: boolean;
  ctrlKey: boolean;
  altKey: boolean;
  shiftKey: boolean;
}

export interface ShortcutDefinition {
  id: string;
  code?: string;
  key?: string;
  displayKey: string;
  acceptedCodes?: string[];
  acceptedKeys?: string[];
  allowShift?: boolean;
}

export const APP_SHORTCUTS = {
  toggleSidebar: {
    id: "sidebar.toggle",
    code: "KeyB",
    key: "b",
    displayKey: "B",
  },
  toggleSettings: {
    id: "settings.toggle",
    code: "Comma",
    key: ",",
    displayKey: ",",
  },
  increaseLyricsFont: {
    id: "lyrics.font.increase",
    displayKey: "+",
    acceptedCodes: ["Equal", "NumpadAdd"],
    acceptedKeys: ["+", "="],
    allowShift: true,
  },
  decreaseLyricsFont: {
    id: "lyrics.font.decrease",
    code: "Minus",
    key: "-",
    displayKey: "-",
  },
  resetLyricsFont: {
    id: "lyrics.font.reset",
    code: "Digit0",
    key: "0",
    displayKey: "0",
  },
} satisfies Record<string, ShortcutDefinition>;

export function getShortcutPlatform(): ShortcutPlatform {
  const platform =
    typeof navigator !== "undefined"
      ? (navigator as Navigator & { userAgentData?: { platform?: string } })
          .userAgentData?.platform || navigator.platform
      : "";

  if (/mac|darwin/i.test(platform)) return "mac";
  if (/win/i.test(platform)) return "windows";
  return "linux";
}

export function isMacShortcutPlatform(
  platform: ShortcutPlatform = getShortcutPlatform(),
): boolean {
  return platform === "mac";
}

export function getShortcutDisplay(
  shortcut: ShortcutDefinition,
  platform: ShortcutPlatform = getShortcutPlatform(),
): string {
  const modifier = platform === "mac" ? "⌘" : "Ctrl+";
  return `${modifier}${shortcut.displayKey}`;
}

export function matchesShortcut(
  shortcut: ShortcutDefinition,
  event: ShortcutEventLike,
): boolean {
  const acceptedCodes =
    shortcut.acceptedCodes ?? (shortcut.code ? [shortcut.code] : []);
  const acceptedKeys =
    shortcut.acceptedKeys ?? (shortcut.key ? [shortcut.key] : []);

  return (
    (event.metaKey || event.ctrlKey) &&
    !event.altKey &&
    (shortcut.allowShift || !event.shiftKey) &&
    acceptedCodes.includes(event.code) &&
    acceptedKeys.some((key) => event.key.toLowerCase() === key.toLowerCase())
  );
}

export function isEditableShortcutTarget(target: EventTarget | null): boolean {
  const element = target as {
    tagName?: string;
    isContentEditable?: boolean;
  } | null;

  if (!element) {
    return false;
  }

  return (
    element.tagName === "INPUT" ||
    element.tagName === "TEXTAREA" ||
    element.tagName === "SELECT" ||
    element.isContentEditable === true
  );
}
