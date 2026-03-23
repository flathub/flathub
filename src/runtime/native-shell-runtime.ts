import { useEffect } from "react";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import * as api from "@/lib/tauri";
import { useLayoutStore } from "@/stores/layout-store";

export function useNativeSidebarVisibilitySync(enabled: boolean): void {
  const sidebarVisible = useLayoutStore((s) => s.sidebarVisible);

  useEffect(() => {
    if (!enabled || getShortcutPlatform() !== "mac") {
      return;
    }

    void api.setNativeSidebarVisibility(sidebarVisible).catch(() => undefined);
  }, [enabled, sidebarVisible]);
}
