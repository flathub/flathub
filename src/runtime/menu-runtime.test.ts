import { describe, expect, test, vi } from "vitest";
import {
  handleAppMenuAction,
  promptImportFiles,
  type AppMenuAction,
} from "./menu-runtime";

describe("app menu runtime", () => {
  test("opens settings when the settings menu item is selected", async () => {
    const openSettings = vi.fn();
    const importFromDialog = vi.fn();

    await handleAppMenuAction("open-settings", {
      openSettings,
      importFromDialog,
    });

    expect(openSettings).toHaveBeenCalledOnce();
    expect(importFromDialog).not.toHaveBeenCalled();
  });

  test("opens the import dialog when the import menu item is selected", async () => {
    const openSettings = vi.fn();
    const importFromDialog = vi.fn().mockResolvedValue(undefined);

    await handleAppMenuAction("import-files", {
      openSettings,
      importFromDialog,
    });

    expect(importFromDialog).toHaveBeenCalledOnce();
    expect(openSettings).not.toHaveBeenCalled();
  });

  test("imports every selected path from the shared file picker", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);
    const selectedPaths = ["/music/one.mp3", "/music/two.lrc"];

    await promptImportFiles({
      importFiles,
      openDialog: vi.fn().mockResolvedValue(selectedPaths),
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
    });

    expect(importFiles).toHaveBeenCalledWith(selectedPaths);
  });

  test("does nothing when the shared file picker is cancelled", async () => {
    const importFiles = vi.fn().mockResolvedValue(undefined);

    await promptImportFiles({
      importFiles,
      openDialog: vi.fn().mockResolvedValue(null),
      getDefaultAudioDir: vi.fn().mockResolvedValue("/music"),
    });

    expect(importFiles).not.toHaveBeenCalled();
  });

  test("ignores unknown menu actions", async () => {
    const openSettings = vi.fn();
    const importFromDialog = vi.fn();

    await handleAppMenuAction("unknown" as AppMenuAction, {
      openSettings,
      importFromDialog,
    });

    expect(openSettings).not.toHaveBeenCalled();
    expect(importFromDialog).not.toHaveBeenCalled();
  });
});
