import { LogicalPosition } from "@tauri-apps/api/dpi";
import { MenuItem, PredefinedMenuItem, Submenu } from "@tauri-apps/api/menu";
import type { TFunction } from "i18next";

export type DesktopMenuKey = "edit" | "file" | "help" | "window";

export interface DesktopMenuResources {
  edit: Submenu;
  file: Submenu;
  help: Submenu;
  window: Submenu;
}

interface CreateDesktopTitlebarMenusOptions {
  onImportFiles: () => void | Promise<void>;
  onOpenSettings: () => void | Promise<void>;
  t: TFunction;
}

export async function createDesktopTitlebarMenus({
  onImportFiles,
  onOpenSettings,
  t,
}: CreateDesktopTitlebarMenusOptions): Promise<DesktopMenuResources> {
  const file = await Submenu.new({
    text: t("windowChrome.file"),
    items: [
      await MenuItem.new({
        accelerator: "CmdOrCtrl+O",
        action: () => {
          void onImportFiles();
        },
        id: "window.file.import",
        text: t("windowChrome.import"),
      }),
      await MenuItem.new({
        accelerator: "CmdOrCtrl+,",
        action: () => {
          void onOpenSettings();
        },
        id: "window.file.settings",
        text: t("windowChrome.settings"),
      }),
      await PredefinedMenuItem.new({ item: "Separator" }),
      await PredefinedMenuItem.new({
        item: "CloseWindow",
        text: t("windowChrome.closeWindow"),
      }),
    ],
  });

  const edit = await Submenu.new({
    text: t("windowChrome.edit"),
    items: [
      await PredefinedMenuItem.new({
        item: "Undo",
        text: t("windowChrome.undo"),
      }),
      await PredefinedMenuItem.new({
        item: "Redo",
        text: t("windowChrome.redo"),
      }),
      await PredefinedMenuItem.new({ item: "Separator" }),
      await PredefinedMenuItem.new({
        item: "Cut",
        text: t("windowChrome.cut"),
      }),
      await PredefinedMenuItem.new({
        item: "Copy",
        text: t("windowChrome.copy"),
      }),
      await PredefinedMenuItem.new({
        item: "Paste",
        text: t("windowChrome.paste"),
      }),
      await PredefinedMenuItem.new({
        item: "SelectAll",
        text: t("windowChrome.selectAll"),
      }),
    ],
  });

  const window = await Submenu.new({
    text: t("windowChrome.window"),
    items: [
      await PredefinedMenuItem.new({
        item: "Minimize",
        text: t("windowChrome.minimize"),
      }),
      await PredefinedMenuItem.new({
        item: "Maximize",
        text: t("windowChrome.maximize"),
      }),
      await PredefinedMenuItem.new({ item: "Separator" }),
      await PredefinedMenuItem.new({
        item: "CloseWindow",
        text: t("windowChrome.closeWindow"),
      }),
    ],
  });

  const help = await Submenu.new({
    text: t("windowChrome.help"),
    items: [
      await PredefinedMenuItem.new({
        item: { About: null },
        text: t("windowChrome.about"),
      }),
    ],
  });

  return { edit, file, help, window };
}

export async function destroyDesktopTitlebarMenus(
  menus: Partial<DesktopMenuResources> | null,
): Promise<void> {
  if (!menus) {
    return;
  }

  await Promise.all(
    Object.values(menus).map((menu) => menu?.close().catch(() => undefined)),
  );
}

export function getDesktopMenuPopupPosition(anchor: HTMLElement) {
  const rect = anchor.getBoundingClientRect();
  return new LogicalPosition(Math.round(rect.left), Math.round(rect.bottom));
}
