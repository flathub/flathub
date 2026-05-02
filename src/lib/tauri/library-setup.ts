import { invoke } from "@tauri-apps/api/core";
import type { LibraryRegistrySnapshot } from "@/types/ipc";

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

export function renameLibrary(
  libraryId: string,
  displayName: string,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("rename_library", {
    libraryId,
    displayName,
  });
}

export function deleteLibrary(
  libraryId: string,
): Promise<LibraryRegistrySnapshot> {
  return invoke<LibraryRegistrySnapshot>("delete_library", {
    libraryId,
  });
}

export function createLibrary(path: string): Promise<void> {
  return createLocalLibrary(path);
}

export function openLibrary(path: string): Promise<void> {
  return registerLocalLibrary(path);
}
