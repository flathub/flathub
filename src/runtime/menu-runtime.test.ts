import { describe, expect, test, vi } from "vitest";
import {
  handleAppMenuAction,
  promptImportFiles,
  type AppMenuAction,
} from "./menu-runtime";

const { mockGetShortcutPlatform } = vi.hoisted(() => ({
  mockGetShortcutPlatform: vi.fn(() => "mac"),
}));

vi.mock("@/lib/app-shortcuts", async () => {
  const actual = await vi.importActual<typeof import("@/lib/app-shortcuts")>(
    "@/lib/app-shortcuts",
  );

  return {
    ...actual,
    getShortcutPlatform: mockGetShortcutPlatform,
  };
});

describe("app menu runtime", () => {
  test("opens settings when the settings menu item is selected", async () => {
    mockGetShortcutPlatform.mockReturnValue("mac");
    const toggleSettings = vi.fn();
    const importFromDialog = vi.fn();
    const toggleSidebar = vi.fn();

    await handleAppMenuAction("open-settings", {
      toggleSettings,
      importFromDialog,
      toggleSidebar,
    });

    expect(toggleSettings).toHaveBeenCalledOnce();
    expect(importFromDialog).not.toHaveBeenCalled();
    expect(toggleSidebar).not.toHaveBeenCalled();
  });

  test("opens settings when the switch library menu item is selected", async () => {
    mockGetShortcutPlatform.mockReturnValue("mac");
    const toggleSettings = vi.fn();
    const importFromDialog = vi.fn();
    const toggleSidebar = vi.fn();

    await handleAppMenuAction("switch-library", {
      toggleSettings,
      importFromDialog,
      toggleSidebar,
    });

    expect(toggleSettings).toHaveBeenCalledOnce();
    expect(importFromDialog).not.toHaveBeenCalled();
    expect(toggleSidebar).not.toHaveBeenCalled();
  });

  test("opens the import dialog when the import menu item is selected", async () => {
    mockGetShortcutPlatform.mockReturnValue("mac");
    const toggleSettings = vi.fn();
    const importFromDialog = vi.fn().mockResolvedValue(undefined);
    const toggleSidebar = vi.fn();

    await handleAppMenuAction("import-files", {
      toggleSettings,
      importFromDialog,
      toggleSidebar,
    });

    expect(importFromDialog).toHaveBeenCalledOnce();
    expect(toggleSettings).not.toHaveBeenCalled();
    expect(toggleSidebar).not.toHaveBeenCalled();
  });

  test("toggles the sidebar when the native shell requests it", async () => {
    mockGetShortcutPlatform.mockReturnValue("mac");
    const toggleSettings = vi.fn();
    const importFromDialog = vi.fn().mockResolvedValue(undefined);
    const toggleSidebar = vi.fn();

    await handleAppMenuAction("toggle-sidebar", {
      toggleSettings,
      importFromDialog,
      toggleSidebar,
    });

    expect(toggleSidebar).toHaveBeenCalledOnce();
    expect(importFromDialog).not.toHaveBeenCalled();
    expect(toggleSettings).not.toHaveBeenCalled();
  });

  test("imports every selected path from the shared file picker", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);
    const selectedPaths = ["/music/one.mp3", "/music/two.lrc"];
    mockGetShortcutPlatform.mockReturnValue("windows");

    await promptImportFiles({
      importFiles,
      openDialog: vi.fn().mockResolvedValue(selectedPaths),
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
      pickImportPaths: vi.fn(),
      expandImportPaths: vi.fn().mockResolvedValue({
        paths: selectedPaths,
        song_count: 1,
      }),
      confirmImport: vi.fn().mockResolvedValue(true),
    });

    expect(importFiles).toHaveBeenCalledWith(selectedPaths);
  });

  test("does nothing when the shared file picker is cancelled", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);
    mockGetShortcutPlatform.mockReturnValue("windows");

    await promptImportFiles({
      importFiles,
      openDialog: vi.fn().mockResolvedValue(null),
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
      pickImportPaths: vi.fn(),
      expandImportPaths: vi.fn(),
      confirmImport: vi.fn(),
    });

    expect(importFiles).not.toHaveBeenCalled();
  });

  test("uses the mixed picker on macOS and confirms the expanded song count", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);
    const selectedFolders = ["/music/library"];
    const expandedPaths = [
      "/music/library/track-a.mp3",
      "/music/library/nested/track-b.flac",
      "/music/library/nested/deeper/track-c.zip",
    ];
    mockGetShortcutPlatform.mockReturnValue("mac");
    const pickImportPaths = vi.fn().mockResolvedValue(selectedFolders);
    const expandImportPaths = vi.fn().mockResolvedValue({
      paths: expandedPaths,
      song_count: 3,
    });
    const confirmImport = vi.fn().mockResolvedValue(true);

    await promptImportFiles({
      importFiles,
      openDialog: vi.fn(),
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
      pickImportPaths,
      expandImportPaths,
      confirmImport,
    });

    expect(pickImportPaths).toHaveBeenCalledWith("/music");
    expect(expandImportPaths).toHaveBeenCalledWith(selectedFolders);
    expect(confirmImport).toHaveBeenCalledWith(
      expect.stringContaining("3"),
      expect.any(Object),
    );
    expect(importFiles).toHaveBeenCalledWith(expandedPaths);
  });

  test("does nothing when the mixed picker is cancelled on macOS", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);
    const expandImportPaths = vi.fn();
    const confirmImport = vi.fn();
    mockGetShortcutPlatform.mockReturnValue("mac");

    await promptImportFiles({
      importFiles,
      openDialog: vi.fn(),
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
      pickImportPaths: vi.fn().mockResolvedValue([]),
      expandImportPaths,
      confirmImport,
    });

    expect(expandImportPaths).not.toHaveBeenCalled();
    expect(confirmImport).not.toHaveBeenCalled();
    expect(importFiles).not.toHaveBeenCalled();
  });

  test("stops when the import confirmation is declined", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);
    const openDialog = vi.fn().mockResolvedValue(["/music/track.mp3"]);
    mockGetShortcutPlatform.mockReturnValue("windows");
    const expandImportPaths = vi.fn().mockResolvedValue({
      paths: ["/music/track.mp3"],
      song_count: 1,
    });
    const confirmImport = vi.fn().mockResolvedValue(false);

    await promptImportFiles({
      importFiles,
      openDialog,
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
      pickImportPaths: vi.fn(),
      expandImportPaths,
      confirmImport,
    });

    expect(importFiles).not.toHaveBeenCalled();
  });

  test("falls back to the stock file picker on non-mac platforms without any source prompt", async () => {
    mockGetShortcutPlatform.mockReturnValue("windows");
    const openDialog = vi.fn().mockResolvedValue(["/music/track.mp3"]);
    const pickImportPaths = vi.fn();

    await promptImportFiles({
      importFiles: vi.fn().mockResolvedValue(undefined),
      openDialog,
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
      pickImportPaths,
      expandImportPaths: vi.fn().mockResolvedValue({
        paths: ["/music/track.mp3"],
        song_count: 1,
      }),
      confirmImport: vi.fn().mockResolvedValue(false),
    });

    expect(openDialog).toHaveBeenCalledWith(
      expect.objectContaining({
        multiple: true,
        defaultPath: "/music",
      }),
    );
    expect(pickImportPaths).not.toHaveBeenCalled();
  });

  test("ignores unknown menu actions", async () => {
    mockGetShortcutPlatform.mockReturnValue("mac");
    const toggleSettings = vi.fn();
    const importFromDialog = vi.fn();
    const toggleSidebar = vi.fn();

    await handleAppMenuAction("unknown" as AppMenuAction, {
      toggleSettings,
      importFromDialog,
      toggleSidebar,
    });

    expect(toggleSettings).not.toHaveBeenCalled();
    expect(importFromDialog).not.toHaveBeenCalled();
    expect(toggleSidebar).not.toHaveBeenCalled();
  });
});
