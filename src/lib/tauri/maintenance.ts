import { invoke } from "@tauri-apps/api/core";
import type {
  DeleteStemsResult,
  DowngradeResult,
  ExtractEmbeddedCoverArtResult,
  SeparationStatusSnapshot,
} from "@/types/ipc";

export function deleteAllStems(): Promise<DeleteStemsResult> {
  return invoke<DeleteStemsResult>("delete_all_stems");
}

export function estimateStemsSize(): Promise<number> {
  return invoke<number>("estimate_stems_size");
}

export function deleteAllCachedLyrics(): Promise<number> {
  return invoke<number>("delete_all_cached_lyrics");
}

export function extractEmbeddedCoverArt(
  songIds: string[],
): Promise<ExtractEmbeddedCoverArtResult> {
  return invoke<ExtractEmbeddedCoverArtResult>("extract_embedded_cover_art", {
    songIds,
  });
}

export function batchSeparate(songIds: string[]): Promise<void> {
  return invoke<void>("batch_separate", { songIds });
}

export function cancelBatchSeparation(): Promise<void> {
  return invoke<void>("cancel_batch_separation");
}

export function downgradeToTwoStem(
  songId: string,
): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("downgrade_single_to_two_stem", {
    songId,
  });
}

export function downgradeAllToTwoStem(): Promise<DowngradeResult> {
  return invoke<DowngradeResult>("downgrade_all_to_two_stem");
}

export function estimateDowngradeSavings(): Promise<number> {
  return invoke<number>("estimate_downgrade_savings");
}
