import { invoke } from "@tauri-apps/api/core";
import type {
  AirPlayAudienceStatePayload,
  AirPlayRoutePickerBounds,
  AppSettings,
  LibraryRegistrySnapshot,
  RemoteAuthPayload,
  RemoteAuthStart,
  RemoteAuthStatus,
  RemoteLibraryCandidate,
  RemoteLibraryProvider,
  DeleteSongsResult,
  DeleteStemsResult,
  DowngradeResult,
  ExecutionProvider,
  ExtractEmbeddedCoverArtResult,
  ExpandedImportPaths,
  ImportCandidateDetails,
  ImportLyricsResult,
  ImportSongsOptions,
  ImportSongsResult,
  LyricsPayload,
  ModelBootstrapStatusSnapshot,
  ModelStatusSnapshot,
  PlaybackStateSnapshot,
  SeparationStatusSnapshot,
  StemName,
  Song,
  SongProperties,
  WindowShellStateSnapshot,
} from "@/types/ipc";

// ─── Library Setup ───────────────────────────────────────

export function getLibraryPath(): Promise<string | null> {
  return invoke<string | null>("get_library_path");
}

export function getLibraryRegistry(): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("get_library_registry");
}

export function getActiveLibrary(): Promise<
  LibraryRegistrySnapshot["libraries"][number] | null
> {
  return invoke<LibraryRegistrySnapshot["libraries"][number] | null>(
    "get_active_library",
  );
}

export function createLocalLibrary(path: string): Promise<void> {
  return invoke<void>("create_library", { path });
}

export function registerLocalLibrary(path: string): Promise<void> {
  return invoke<void>("open_library", { path });
}

export function beginRemoteAuth(
  provider: RemoteLibraryProvider,
  payload: RemoteAuthPayload = null,
): Promise<RemoteAuthStart> {
  return invoke<RemoteAuthStart>("begin_remote_auth", {
    provider,
    payload,
  });
}

export function pollRemoteAuth(sessionId: string): Promise<RemoteAuthStatus> {
  return invoke<RemoteAuthStatus>("poll_remote_auth", {
    sessionId,
  });
}

export function listRemoteLibraryRoots(
  sessionId: string,
): Promise<RemoteLibraryCandidate[]> {
  return invoke<RemoteLibraryCandidate[]>("list_remote_library_roots", {
    sessionId,
  });
}

export function createRemoteLibrary(
  sessionId: string,
  displayName: string,
): Promise<RemoteLibraryCandidate> {
  return invoke<RemoteLibraryCandidate>("create_remote_library", {
    sessionId,
    displayName,
  });
}

export function registerRemoteLibrary(
  sessionId: string,
  remoteRootLocator: string,
  displayName?: string | null,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("register_remote_library", {
    sessionId,
    remoteRootLocator,
    displayName: displayName ?? null,
  });
}

export function setRemoteMirror(
  localLibraryId: string,
  remoteLibraryId: string | null,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("set_remote_mirror", {
    localLibraryId,
    remoteLibraryId,
  });
}

export function syncActiveRemoteLibrary(): Promise<unknown> {
  return invoke<unknown>("sync_active_remote_library");
}

export function publishSongToRemote(songId: string): Promise<unknown> {
  return invoke<unknown>("publish_song_to_remote", { songId });
}

export function publishSongsToRemote(songIds: string[]): Promise<unknown> {
  return invoke<unknown>("publish_songs_to_remote", { songIds });
}

export function getAllUploadStatuses(): Promise<unknown[]> {
  return invoke<unknown[]>("get_all_upload_statuses");
}

export function switchLibrary(
  libraryId: string,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("switch_library", {
    libraryId,
  });
}

export function removeLibrary(
  libraryId: string,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("remove_library", {
    libraryId,
  });
}

export function createLibrary(path: string): Promise<void> {
  return createLocalLibrary(path);
}

export function openLibrary(path: string): Promise<void> {
  return registerLocalLibrary(path);
}

// ─── Library ─────────────────────────────────────────────

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

export function deleteSongs(songIds: string[]): Promise<DeleteSongsResult> {
  return invoke<DeleteSongsResult>("delete_songs", { songIds });
}

export function getSongProperties(songId: string): Promise<SongProperties> {
  return invoke<SongProperties>("get_song_properties", { songId });
}

// ─── Playback ────────────────────────────────────────────

export function play(songId: string): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("play", { songId });
}

export function resume(): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("resume");
}

export function pause(): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("pause");
}

export function seek(ms: number): Promise<PlaybackStateSnapshot> {
  return invoke<PlaybackStateSnapshot>("seek", { ms: Math.round(ms) });
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

export function syncAirPlayRoutePicker(
  bounds: AirPlayRoutePickerBounds | null,
): Promise<void> {
  return invoke<void>("sync_airplay_route_picker", { bounds });
}

export function syncAirPlayAudienceState(
  payload: AirPlayAudienceStatePayload,
): Promise<void> {
  return invoke<void>("sync_airplay_audience_state", { payload });
}

export function stepAirPlayPlainTextPage(
  direction: "prev" | "next",
): Promise<void> {
  return invoke<void>("step_airplay_plain_text_page", { direction });
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

export function getAllSeparationStatuses(): Promise<
  SeparationStatusSnapshot[]
> {
  return invoke<SeparationStatusSnapshot[]>("get_all_separation_statuses");
}

// ─── Lyrics ──────────────────────────────────────────────

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

// ─── Bootstrap ───────────────────────────────────────────

export function getModelBootstrapStatus(): Promise<ModelBootstrapStatusSnapshot> {
  return invoke<ModelBootstrapStatusSnapshot>("get_model_bootstrap_status");
}

// ─── Settings ───────────────────────────────────────────

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

// ─── Maintenance ────────────────────────────────────────

export function deleteAllStems(): Promise<DeleteStemsResult> {
  return invoke<DeleteStemsResult>("delete_all_stems");
}

export function estimateStemsSize(): Promise<number> {
  return invoke<number>("estimate_stems_size");
}

export function deleteAllCachedLyrics(): Promise<number> {
  return invoke<number>("delete_all_cached_lyrics");
}

export function extractEmbeddedLyrics(songId: string): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("extract_embedded_lyrics", { songId });
}

export function extractEmbeddedCoverArt(
  songIds: string[],
): Promise<ExtractEmbeddedCoverArtResult> {
  return invoke<ExtractEmbeddedCoverArtResult>("extract_embedded_cover_art", {
    songIds,
  });
}

export function fetchLyricsOnline(songId: string): Promise<LyricsPayload> {
  return invoke<LyricsPayload>("fetch_lyrics_online", { songId });
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

// ─── CDG ────────────────────────────────────────────────

/**
 * Returns a raw RGBA frame (288×192) as an `ArrayBuffer` for the given
 * playback position. An empty buffer (`byteLength === 0`) means no CDG is
 * active or the frame hasn't changed.
 *
 * PERF: The backend returns raw bytes via `tauri::ipc::Response`, which the
 * IPC bridge delivers as an `ArrayBuffer` — no base64 encoding/decoding is
 * involved. This is a deliberate performance choice: base64 inflates the
 * payload by ~33% and requires an expensive O(n) decode loop on the main
 * thread. Do not change the return type to `string` without benchmarking.
 */
export function getCdgFrame(ms: number): Promise<ArrayBuffer> {
  return invoke<ArrayBuffer>("get_cdg_frame", {
    positionMs: Math.round(ms),
  });
}
