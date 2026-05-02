import { invoke } from "@tauri-apps/api/core";
import type {
  LibraryRegistrySnapshot,
  RemoteAuthPayload,
  RemoteAuthStart,
  RemoteAuthStatus,
  RemoteLibraryCandidate,
  RemoteLibraryProvider,
  UploadStatusSnapshot,
} from "@/types/ipc";

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

export function cancelRemoteAuth(sessionId: string): Promise<void> {
  return invoke<void>("cancel_remote_auth", {
    sessionId,
  });
}

export function openExternalUrl(url: string): Promise<void> {
  return invoke<void>("open_external_url", { url });
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

export function resolveRemoteLibraryCandidate(
  sessionId: string,
  displayName: string,
): Promise<RemoteLibraryCandidate> {
  return invoke<RemoteLibraryCandidate>("resolve_remote_library_candidate", {
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

export function reauthorizeRemoteLibrary(
  libraryId: string,
  sessionId: string,
  remoteRootLocator: string,
  displayName: string,
  allowRelocation: boolean,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("reauthorize_remote_library", {
    libraryId,
    sessionId,
    remoteRootLocator,
    displayName,
    allowRelocation,
  });
}

export function mirrorLocalLibraryToRemote(
  localLibraryId: string,
  remoteLibraryId: string,
): Promise<void> {
  return invoke<void>("mirror_local_library_to_remote", {
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

export function getAllUploadStatuses(): Promise<UploadStatusSnapshot[]> {
  return invoke<UploadStatusSnapshot[]>("get_all_upload_statuses");
}
