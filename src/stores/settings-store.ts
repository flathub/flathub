import { create } from "zustand";
import { notifyError } from "@/lib/errors";
import * as api from "@/lib/tauri";
import type { AppSettings, ModelVariant, StemMode } from "@/types/ipc";

export interface AppSettingsSnapshot {
  hydrated: boolean;
  stemMode: StemMode;
  modelVariant: ModelVariant;
  language: string | null;
  hideBatchSeparate: boolean;
  lyricsFontStep: number;
}

interface SettingsState {
  isOpen: boolean;
  hydrated: AppSettingsSnapshot["hydrated"];
  stemMode: AppSettingsSnapshot["stemMode"];
  modelVariant: AppSettingsSnapshot["modelVariant"];
  language: AppSettingsSnapshot["language"];
  hideBatchSeparate: AppSettingsSnapshot["hideBatchSeparate"];
  lyricsFontStep: AppSettingsSnapshot["lyricsFontStep"];
  toggle: () => void;
  close: () => void;
  open: () => void;
  hydrateAppSettings: (settings: AppSettings) => void;
  patchAppSettings: (patch: Partial<AppSettingsSnapshot>) => void;
  setLyricsFontStep: (step: number) => Promise<void>;
  adjustLyricsFontStep: (delta: number) => Promise<void>;
  resetLyricsFontStep: () => Promise<void>;
  getAppSettingsSnapshot: () => AppSettingsSnapshot;
}

const DEFAULT_APP_SETTINGS: AppSettingsSnapshot = {
  hydrated: false,
  stemMode: "two_stem",
  modelVariant: "htdemucs",
  language: null,
  hideBatchSeparate: false,
  lyricsFontStep: 0,
};

function toAppSettingsSnapshot(settings: AppSettings): AppSettingsSnapshot {
  return {
    hydrated: true,
    stemMode: settings.stem_mode,
    modelVariant: settings.model_variant,
    language: settings.language,
    hideBatchSeparate: settings.hide_batch_separate,
    lyricsFontStep: settings.lyrics_font_step,
  };
}

function selectAppSettingsSnapshot(
  state: AppSettingsSnapshot,
): AppSettingsSnapshot {
  return {
    hydrated: state.hydrated,
    stemMode: state.stemMode,
    modelVariant: state.modelVariant,
    language: state.language,
    hideBatchSeparate: state.hideBatchSeparate,
    lyricsFontStep: state.lyricsFontStep,
  };
}

export const useSettingsStore = create<SettingsState>((set, get) => ({
  isOpen: false,
  ...DEFAULT_APP_SETTINGS,
  toggle: () => set((s) => ({ isOpen: !s.isOpen })),
  close: () => set({ isOpen: false }),
  open: () => set({ isOpen: true }),
  hydrateAppSettings: (settings) => set(toAppSettingsSnapshot(settings)),
  patchAppSettings: (patch) => set(patch),
  setLyricsFontStep: async (step) => {
    try {
      const settings = await api.setLyricsFontStep(step);
      set(toAppSettingsSnapshot(settings));
    } catch (error) {
      notifyError(error);
    }
  },
  adjustLyricsFontStep: async (delta) => {
    const current = get().lyricsFontStep;
    const nextStep = Math.max(-2, Math.min(2, current + delta));
    if (nextStep === current) {
      return;
    }
    await get().setLyricsFontStep(nextStep);
  },
  resetLyricsFontStep: async () => {
    if (get().lyricsFontStep === 0) {
      return;
    }
    await get().setLyricsFontStep(0);
  },
  getAppSettingsSnapshot: () => selectAppSettingsSnapshot(get()),
}));
