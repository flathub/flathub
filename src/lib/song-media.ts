import type { Song } from "@/types/ipc";

export function songHasCdgMedia(song: Song | null | undefined): boolean {
  if (!song) {
    return false;
  }

  return song.media_g_container === "zip" || song.cdg_path !== null;
}

export function songSupportsInstrumentalFlag(
  song: Song | null | undefined,
): boolean {
  return !songHasCdgMedia(song);
}

export function songCanBeSeparated(song: Song | null | undefined): boolean {
  if (!song || !songSupportsInstrumentalFlag(song)) {
    return false;
  }

  return !song.instrumental;
}
