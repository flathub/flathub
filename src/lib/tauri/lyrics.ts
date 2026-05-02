import { invoke } from "@tauri-apps/api/core";
import type { ImportLyricsResult, LyricsPayload } from "@/types/ipc";

export function importLyricsFiles(
  paths: string[],
): Promise<ImportLyricsResult> {
  return invoke<ImportLyricsResult>("import_lyrics_files", { paths });
}

export function fetchLyrics(songId: string): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("fetch_lyrics", { songId });
}

export function setLyricsOffset(songId: string, ms: number): Promise<void> {
  return invoke<void>("set_lyrics_offset", { songId, ms });
}

export function saveManualLyrics(
  songId: string,
  text: string,
): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("save_manual_lyrics", { songId, text });
}

export function extractEmbeddedLyrics(songId: string): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("extract_embedded_lyrics", { songId });
}

export function fetchLyricsOnline(songId: string): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("fetch_lyrics_online", { songId });
}
