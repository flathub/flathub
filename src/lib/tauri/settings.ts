import { invoke } from "@tauri-apps/api/core";
import type {
  AppSettings,
  ExecutionProvider,
  ModelBootstrapStatusSnapshot,
  ModelStatusSnapshot,
  WindowShellStateSnapshot,
} from "@/types/ipc";

export function getModelBootstrapStatus(): Promise<ModelBootstrapStatusSnapshot> {
  return invoke<ModelBootstrapStatusSnapshot>("get_model_bootstrap_status");
}

export function getSettings(): Promise<AppSettings> {
  return invoke<AppSettings>("get_settings");
}

export function getWindowShellState(): Promise<WindowShellStateSnapshot> {
  return invoke<WindowShellStateSnapshot>("get_window_shell_state");
}

export function setNativeSidebarVisibility(visible: boolean): Promise<void> {
  return invoke<void>("set_native_sidebar_visibility", { visible });
}

export function setStemMode(mode: string): Promise<AppSettings> {
  return invoke<AppSettings>("set_stem_mode", { mode });
}

export function setModelVariant(variant: string): Promise<AppSettings> {
  return invoke<AppSettings>("set_model_variant", { variant });
}

export function downloadModel(
  variant: string,
): Promise<ModelBootstrapStatusSnapshot> {
  return invoke<ModelBootstrapStatusSnapshot>("download_model", { variant });
}

export function deleteModel(variant: string): Promise<void> {
  return invoke<void>("delete_model", { variant });
}

export function getModelStatus(variant: string): Promise<ModelStatusSnapshot> {
  return invoke<ModelStatusSnapshot>("get_model_status", { variant });
}

export function setLanguage(language: string): Promise<AppSettings> {
  return invoke<AppSettings>("set_language", { language });
}

export function setHideBatchSeparate(value: boolean): Promise<AppSettings> {
  return invoke<AppSettings>("set_hide_batch_separate", { value });
}

export function setCoverArtBackdrop(value: boolean): Promise<AppSettings> {
  return invoke<AppSettings>("set_cover_art_backdrop", { value });
}

export function setExecutionProvider(
  provider: ExecutionProvider,
): Promise<AppSettings> {
  return invoke<AppSettings>("set_execution_provider", { provider });
}

export function setLyricsFontStep(step: number): Promise<AppSettings> {
  return invoke<AppSettings>("set_lyrics_font_step", { step });
}

export function restartApp(): Promise<void> {
  return invoke<void>("restart_app");
}
