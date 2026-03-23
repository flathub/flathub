import { SongListItem } from "./SongListItem";
import { EmptyLibrary } from "./EmptyLibrary";
import { useLibraryStore } from "@/stores/library-store";

interface SongListProps {
  variant?: "default" | "native";
}

export function SongList({ variant = "default" }: SongListProps = {}) {
  const songs = useLibraryStore((s) => s.songs);
  const filter = useLibraryStore((s) => s.filter);
  const separationStatuses = useLibraryStore((s) => s.separationStatuses);

  const filteredSongs =
    filter === "separated"
      ? songs.filter((s) => separationStatuses[s.hash]?.state === "completed")
      : songs;

  const orderedHashes = filteredSongs.map((s) => s.hash);

  if (filteredSongs.length === 0) {
    return <EmptyLibrary />;
  }

  return (
    <div
      className={`custom-scrollbar flex-1 overflow-y-auto ${
        variant === "native" ? "space-y-1" : "space-y-0.5"
      }`}
      data-song-list-visual-variant={variant}
    >
      {filteredSongs.map((song) => (
        <SongListItem
          key={song.hash}
          song={song}
          orderedHashes={orderedHashes}
          variant={variant}
        />
      ))}
    </div>
  );
}
