import type {
  PlaybackStateSnapshot,
  SeparationStatusSnapshot,
} from "@/types/ipc";

export interface PlayerWorkflowDependencies {
  play: (songId: string) => Promise<PlaybackStateSnapshot>;
  loadStems: () => Promise<PlaybackStateSnapshot>;
  getSeparationStatus: (songId: string) => SeparationStatusSnapshot | undefined;
  applySnapshot: (snapshot: PlaybackStateSnapshot) => void;
}

export function shouldEnqueueInsteadOfReplacingCurrentSong(
  currentSnapshot: PlaybackStateSnapshot | null,
  requestedSongId: string,
): boolean {
  return Boolean(
    currentSnapshot?.is_playing &&
    currentSnapshot.song_id &&
    currentSnapshot.song_id !== requestedSongId,
  );
}

export function shouldLoadSeparatedStems(
  snapshot: PlaybackStateSnapshot,
  separationStatus: SeparationStatusSnapshot | undefined,
): boolean {
  return separationStatus?.state === "completed" && !snapshot.has_stems;
}

export async function playTrackWithOptionalStems(
  songId: string,
  dependencies: PlayerWorkflowDependencies,
): Promise<void> {
  const snapshot = await dependencies.play(songId);
  dependencies.applySnapshot(snapshot);

  if (
    !shouldLoadSeparatedStems(
      snapshot,
      dependencies.getSeparationStatus(songId),
    )
  ) {
    return;
  }

  const snapshotWithStems = await dependencies.loadStems();
  dependencies.applySnapshot(snapshotWithStems);
}
