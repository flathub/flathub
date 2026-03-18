import { useEffect } from "react";
import { getCurrentWebviewWindow } from "@tauri-apps/api/webviewWindow";
import { useLibraryStore } from "@/stores/library-store";

export function useFileDrop(enabled = true): void {
  useEffect(() => {
    if (!enabled) {
      return;
    }

    let cancelled = false;

    const setupListener = async () => {
      const webview = getCurrentWebviewWindow();
      const unlisten = await webview.onDragDropEvent((event) => {
        if (cancelled) return;
        if (event.payload.type === "drop") {
          const paths = event.payload.paths;
          if (paths.length > 0) {
            useLibraryStore.getState().importFiles(paths);
          }
        }
      });

      if (cancelled) {
        unlisten();
      } else {
        cleanupRef = unlisten;
      }
    };

    let cleanupRef: (() => void) | undefined;
    setupListener();

    return () => {
      cancelled = true;
      cleanupRef?.();
    };
  }, [enabled]);
}
