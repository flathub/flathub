import type { Song } from "@/types/ipc";

export function songHasCdgMedia(song: Song | null | undefined): boolean {
  if (!song) {
    return false;
  }

  return song.media_g_container === "zip" || song.cdg_path !== null;
}
