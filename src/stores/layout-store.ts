import { create } from "zustand";
import {
  createWebviewSyncChannel,
  type WebviewSyncChannel,
} from "@/runtime/webview-sync";

interface LayoutState {
  sidebarVisible: boolean;
  sidebarWidth: number;
  setSidebarVisible: (visible: boolean) => void;
  setSidebarWidth: (width: number) => void;
  toggleSidebar: () => void;
}

export interface LayoutSyncSnapshot {
  sidebarVisible: boolean;
  sidebarWidth: number;
}

const MIN_SIDEBAR_WIDTH = 240;
const MAX_SIDEBAR_WIDTH = 420;
const DEFAULT_SIDEBAR_WIDTH = 260;

function clampSidebarWidth(width: number): number {
  return Math.min(MAX_SIDEBAR_WIDTH, Math.max(MIN_SIDEBAR_WIDTH, width));
}

export function createLayoutStore(
  syncChannel: WebviewSyncChannel<LayoutSyncSnapshot> = createWebviewSyncChannel<LayoutSyncSnapshot>(
    "openkara.layout",
  ),
) {
  const store = create<LayoutState>((set, get) => {
    const syncLayout = (snapshot: Partial<LayoutSyncSnapshot>) => {
      const current = get();
      const nextVisible = snapshot.sidebarVisible ?? current.sidebarVisible;
      const nextWidth = clampSidebarWidth(
        snapshot.sidebarWidth ?? current.sidebarWidth,
      );

      if (
        current.sidebarVisible === nextVisible &&
        current.sidebarWidth === nextWidth
      ) {
        return;
      }

      set({
        sidebarVisible: nextVisible,
        sidebarWidth: nextWidth,
      });
      syncChannel.publish({
        sidebarVisible: nextVisible,
        sidebarWidth: nextWidth,
      });
    };

    return {
      sidebarVisible: true,
      sidebarWidth: DEFAULT_SIDEBAR_WIDTH,

      setSidebarVisible: (visible) => {
        syncLayout({ sidebarVisible: visible });
      },

      setSidebarWidth: (width) => {
        syncLayout({ sidebarWidth: width });
      },

      toggleSidebar: () => {
        syncLayout({ sidebarVisible: !get().sidebarVisible });
      },
    };
  });

  const unsubscribe = syncChannel.subscribe(
    ({ sidebarVisible, sidebarWidth }) => {
      const current = store.getState();
      const nextWidth = clampSidebarWidth(sidebarWidth);
      if (
        current.sidebarVisible === sidebarVisible &&
        current.sidebarWidth === nextWidth
      ) {
        return;
      }

      store.setState({ sidebarVisible, sidebarWidth: nextWidth });
    },
  );

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
