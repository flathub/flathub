import { create } from "zustand";

interface LayoutState {
  sidebarVisible: boolean;
  setSidebarVisible: (visible: boolean) => void;
  toggleSidebar: () => void;
}

export const useLayoutStore = create<LayoutState>((set) => ({
  sidebarVisible: true,

  setSidebarVisible: (visible) => {
    set({ sidebarVisible: visible });
  },

  toggleSidebar: () => {
    set((state) => ({ sidebarVisible: !state.sidebarVisible }));
  },
}));
