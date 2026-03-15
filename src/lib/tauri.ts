import { invoke } from "@tauri-apps/api/core";
import type {
  AppSettings,
  ImportSongsResult,
  LyricsPayload,
  ModelBootstrapStatusSnapshot,
  PlaybackStateSnapshot,
  SeparationStatusSnapshot,
  StemName,
  Song,
} from "@/types/ipc";

// ─── Library Setup ───────────────────────────────────────

export function getLibraryPath(): Promise<string | null> {
  return invoke<string | null>("get_library_path");
}

export function createLibrary(path: string): Promise<void> {
  return invoke<void>("create_library", { path });
}

export function openLibrary(path: string): Promise<void> {
  return invoke<void>("open_library", { path });
}

// ─── Library ─────────────────────────────────────────────

export function importSongs(paths: string[]): Promise<ImportSongsResult> {
  return invoke<ImportSongsResult>("import_songs", { paths });
}

export function getLibrary(): Promise<Song[]> {
  return invoke<Song[]>("get_library");
}

export function searchLibrary(query: string): Promise<Song[]> {
  return invoke<Song[]>("search_library", { query });
}

export function updateSongMetadata(
  hash: string,
  title: string | null,
  artist: string | null,
): Promise<Song> {
  return invoke<Song>("update_song_metadata", { hash, title, artist });
}

// ─── Playback ────────────────────────────────────────────

export function play(songId: string): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("play", { songId });
}

export function pause(): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("pause");
}

export function seek(ms: number): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("seek", { ms });
}

export function setVolume(level: number): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("set_volume", { level });
}

export function setStemVolume(
  stem: StemName,
  level: number,
): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("set_stem_volume", { stem, level });
}

export function loadStems(): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("load_stems");
}

export function getPlaybackState(): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("get_playback_state");
}

// ─── Separation ──────────────────────────────────────────

export function separate(songId: string): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("separate", { songId });
}

export function getSeparationStatus(
  songId: string,
): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("get_separation_status", { songId });
}

// ─── Lyrics ──────────────────────────────────────────────

export function fetchLyrics(songId: string): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("fetch_lyrics", { songId });
}

export function setLyricsOffset(songId: string, ms: number): Promise<void> {
  return invoke<void>("set_lyrics_offset", { songId, ms });
}

// ─── Bootstrap ───────────────────────────────────────────

export function getModelBootstrapStatus(): Promise<ModelBootstrapStatusSnapshot> {
  return invoke<ModelBootstrapStatusSnapshot>("get_model_bootstrap_status");
}

// ─── Settings ───────────────────────────────────────────

export function getSettings(): Promise<AppSettings> {
  return invoke<AppSettings>("get_settings");
}

export function setStemMode(mode: string): Promise<AppSettings> {
  return invoke<AppSettings>("set_stem_mode", { mode });
}

export function upgradeToFourStem(songId: string): Promise<SeparationStatusSnapshot> {
  return invoke<SeparationStatusSnapshot>("upgrade_to_four_stem", { songId });
}
