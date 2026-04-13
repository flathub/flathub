import { create } from "zustand";
import * as api from "@/lib/tauri";
import type { ModelBootstrapStatusSnapshot } from "@/types/ipc";

interface BootstrapState {
  status: ModelBootstrapStatusSnapshot | null;
  loadStatus: () => Promise<void>;
  updateStatus: (status: ModelBootstrapStatusSnapshot) => void;
}

function mergeBootstrapStatus(
  previous: ModelBootstrapStatusSnapshot | null,
  incoming: ModelBootstrapStatusSnapshot,
): ModelBootstrapStatusSnapshot {
  if (
    previous &&
    incoming.state === "downloading" &&
    previous.state === "downloading" &&
    incoming.model_path === previous.model_path
  ) {
    const prevDown = previous.downloaded_bytes ?? 0;
    const nextDown = Math.max(prevDown, incoming.downloaded_bytes ?? 0);
    const nextTotal = incoming.total_bytes ?? previous.total_bytes ?? null;
    return {
      ...incoming,
      downloaded_bytes: nextDown,
      total_bytes: nextTotal,
    };
  }
  return incoming;
}

export const useBootstrapStore = create<BootstrapState>((set) => ({
  status: null,

  loadStatus: async () => {
    const status = await api.getModelBootstrapStatus();
    set({ status });
  },

  updateStatus: (incoming) =>
    set((s) => ({
      status: mergeBootstrapStatus(s.status, incoming),
    })),
}));
