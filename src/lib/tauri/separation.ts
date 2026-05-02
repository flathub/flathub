import { invoke } from "@tauri-apps/api/core";
import type { SeparationStatusSnapshot } from "@/types/ipc";

export function separate(songId: string): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("separate", { songId });
}

export function getSeparationStatus(
  songId: string,
): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("get_separation_status", { songId });
}

export function getAllSeparationStatuses(): Promise<
  SeparationStatusSnapshot[]
> {
  return invoke<SeparationStatusSnapshot[]>("get_all_separation_statuses");
}

export function upgradeToFourStem(
  songId: string,
): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("upgrade_to_four_stem", { songId });
}

export function reSeparate(
  songId: string,
  stemMode: string,
): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("re_separate", { songId, stemMode });
}
