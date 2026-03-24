import { create } from "zustand";
import {
  createWebviewSyncChannel,
  type WebviewSyncChannel,
} from "@/runtime/webview-sync";

interface LayoutState {
  sidebarVisible: boolean;
  setSidebarVisible: (visible: boolean) => void;
  toggleSidebar: () => void;
}

export interface LayoutSyncSnapshot {
  sidebarVisible: boolean;
}

export function createLayoutStore(
  syncChannel: WebviewSyncChannel<LayoutSyncSnapshot> = createWebviewSyncChannel<LayoutSyncSnapshot>(
    "openkara.layout",
  ),
) {
  const store = create<LayoutState>((set, get) => {
    const syncSidebarVisible = (sidebarVisible: boolean) => {
      if (get().sidebarVisible === sidebarVisible) {
        return;
      }

      set({ sidebarVisible });
      syncChannel.publish({ sidebarVisible });
    };

    return {
      sidebarVisible: true,

      setSidebarVisible: (visible) => {
        syncSidebarVisible(visible);
      },

      toggleSidebar: () => {
        syncSidebarVisible(!get().sidebarVisible);
      },
    };
  });

  const unsubscribe = syncChannel.subscribe(({ sidebarVisible }) => {
    if (store.getState().sidebarVisible === sidebarVisible) {
      return;
    }

    store.setState({ sidebarVisible });
  });

  return {
    store,
    dispose() {
      unsubscribe();
      syncChannel.close();
    },
  };
}

const defaultLayoutStore = createLayoutStore();

export const useLayoutStore = defaultLayoutStore.store;
