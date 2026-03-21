import { useEffect } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { audioDir } from "@tauri-apps/api/path";
import { confirm, open } from "@tauri-apps/plugin-dialog";
import i18next from "@/lib/i18n";
import { notifyError } from "@/lib/errors";
import * as api from "@/lib/tauri";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { useLibraryStore } from "@/stores/library-store";
import { useSettingsStore } from "@/stores/settings-store";

export const APP_MENU_ACTION_EVENT = "openkara://menu-action";

export type AppMenuAction = "import-files" | "open-settings";

export interface ExpandedImportPaths {
  paths: string[];
  song_count: number;
}

interface PromptImportFilesDependencies {
  importFiles: (paths: string[]) => Promise<void>;
  openDialog?: typeof open;
  getDefaultAudioDir?: typeof audioDir;
  confirmImport?: typeof confirm;
  expandImportPaths?: (paths: string[]) => Promise<ExpandedImportPaths>;
  pickImportPaths?: (defaultPath?: string) => Promise<string[]>;
}

interface MenuActionDependencies {
  openSettings: () => void;
  importFromDialog: () => Promise<void>;
}

const IMPORT_FILE_EXTENSIONS = [
  "mp3",
  "flac",
  "wav",
  "ogg",
  "m4a",
  "aac",
  "wma",
  "opus",
  "aiff",
  "aif",
  "cdg",
  "zip",
  "lrc",
];

function formatImportConfirmMessage(songCount: number): string {
  return i18next
    .t("library.importPrompt.confirmMessage")
    .replace("{{songCount}}", String(songCount));
}

function countImportableSongs(paths: string[]): number {
  let songCount = 0;

  for (const path of paths) {
    const normalized = path.replace(/\\/g, "/");
    const extension = normalized
      .split("/")
      .pop()
      ?.split(".")
      .pop()
      ?.toLowerCase();
    if (extension && extension !== "cdg" && extension !== "lrc") {
      songCount += 1;
    }
  }

  return songCount;
}

export async function promptImportFiles({
  importFiles,
  openDialog = open,
  getDefaultAudioDir = audioDir,
  confirmImport = confirm,
  expandImportPaths = api.expandImportPaths,
  pickImportPaths = api.pickImportPaths,
}: PromptImportFilesDependencies): Promise<void> {
  try {
    let defaultPath: string | undefined;

    try {
      defaultPath = await getDefaultAudioDir();
    } catch {
      // audioDir may not be available on all platforms; fall through
    }
    const selectedPaths =
      getShortcutPlatform() === "mac"
        ? await pickImportPaths(defaultPath)
        : await (async () => {
            // RATIONALE: The stock Tauri dialog API cannot pick files and folders
            // from one panel on non-macOS platforms, so the portable fallback stays
            // on direct file picking instead of reintroducing a broken pre-prompt.
            const selected = await openDialog({
              multiple: true,
              defaultPath,
              filters: [
                {
                  name: "Audio & Lyrics",
                  extensions: IMPORT_FILE_EXTENSIONS,
                },
              ],
            });

            return Array.isArray(selected)
              ? selected
              : selected
                ? [selected]
                : [];
          })();

    if (selectedPaths.length === 0) {
      return;
    }

    const expandedSelection = await expandImportPaths(selectedPaths);
    const songCount = countImportableSongs(expandedSelection.paths);
    if (songCount <= 0) {
      return;
    }

    const confirmed = await confirmImport(
      formatImportConfirmMessage(songCount),
      {
        title: i18next.t("library.importPrompt.confirmTitle"),
        kind: "warning",
        okLabel: i18next.t("library.importPrompt.confirmOk"),
        cancelLabel: i18next.t("common.cancel"),
      },
    );

    if (confirmed) {
      await importFiles(expandedSelection.paths);
    }
  } catch (error) {
    notifyError(error);
  }
}

export async function handleAppMenuAction(
  action: AppMenuAction,
  { openSettings, importFromDialog }: MenuActionDependencies,
): Promise<void> {
  switch (action) {
    case "open-settings":
      openSettings();
      return;
    case "import-files":
      await importFromDialog();
      return;
    default:
      return;
  }
}

export function useAppMenuRuntime(enabled: boolean): void {
  const importFiles = useLibraryStore((s) => s.importFiles);
  const openSettings = useSettingsStore((s) => s.open);

  useEffect(() => {
    if (!enabled) {
      return;
    }

    let unlisten: UnlistenFn | undefined;
    let cancelled = false;

    listen<AppMenuAction>(APP_MENU_ACTION_EVENT, (event) => {
      void handleAppMenuAction(event.payload, {
        openSettings,
        importFromDialog: () => promptImportFiles({ importFiles }),
      });
    }).then((dispose) => {
      if (cancelled) {
        dispose();
      } else {
        unlisten = dispose;
      }
    });

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [enabled, importFiles, openSettings]);
}
