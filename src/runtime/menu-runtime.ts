import { useEffect } from "react";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { audioDir } from "@tauri-apps/api/path";
import { open } from "@tauri-apps/plugin-dialog";
import { useLibraryStore } from "@/stores/library-store";
import { useSettingsStore } from "@/stores/settings-store";

export const APP_MENU_ACTION_EVENT = "openkara://menu-action";

export type AppMenuAction = "import-files" | "open-settings";

interface PromptImportFilesDependencies {
  importFiles: (paths: string[]) => Promise<void>;
  openDialog?: typeof open;
  getDefaultAudioDir?: typeof audioDir;
}

interface MenuActionDependencies {
  openSettings: () => void;
  importFromDialog: () => Promise<void>;
}

export async function promptImportFiles({
  importFiles,
  openDialog = open,
  getDefaultAudioDir = audioDir,
}: PromptImportFilesDependencies): Promise<void> {
  let defaultPath: string | undefined;

  try {
    defaultPath = await getDefaultAudioDir();
  } catch {
    // audioDir may not be available on all platforms; fall through
  }

  const selected = await openDialog({
    multiple: true,
    defaultPath,
    filters: [
      {
        name: "Audio & Lyrics",
        extensions: [
          "mp3",
          "flac",
          "wav",
          "ogg",
          "m4a",
          "aac",
          "wma",
          "cdg",
          "zip",
          "lrc",
        ],
      },
    ],
  });

  if (selected && Array.isArray(selected) && selected.length > 0) {
    await importFiles(selected);
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
