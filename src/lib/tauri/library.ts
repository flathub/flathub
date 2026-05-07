import { invoke } from "@tauri-apps/api/core";
import type {
  DeleteSongsResult,
  ExpandedImportPaths,
  ImportCandidateDetails,
  ImportSongsOptions,
  ImportSongsResult,
  Song,
  SongProperties,
} from "@/types/ipc";

export function importSongs(
  paths: string[],
  options?: ImportSongsOptions,
): Promise<ImportSongsResult> {
  return invoke<ImportSongsResult>("import_songs", { paths, options });
}

export function getImportCandidateDetails(
  paths: string[],
): Promise<ImportCandidateDetails[]> {
  return invoke<ImportCandidateDetails[]>("get_import_candidate_details", {
    paths,
  });
}

export function expandImportPaths(
  paths: string[],
): Promise<ExpandedImportPaths> {
  return invoke<ExpandedImportPaths>("expand_import_paths", { paths });
}

export function pickImportPaths(defaultPath?: string): Promise<string[]> {
  return invoke<string[]>("pick_import_paths", {
    defaultPath: defaultPath ?? null,
  });
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

export function setSongsInstrumental(
  songIds: string[],
  instrumental: boolean,
): Promise<Song[]> {
  return invoke<Song[]>("set_songs_instrumental", { songIds, instrumental });
}

export function setSongsLanguage(
  songIds: string[],
  language: string | null,
): Promise<Song[]> {
  return invoke<Song[]>("set_songs_language", { songIds, language });
}

export function deleteSongs(songIds: string[]): Promise<DeleteSongsResult> {
  return invoke<DeleteSongsResult>("delete_songs", { songIds });
}

export function getSongProperties(songId: string): Promise<SongProperties> {
  return invoke<SongProperties>("get_song_properties", { songId });
}
