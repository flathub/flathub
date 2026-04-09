import { SongListItem } from "./SongListItem";
import { EmptyLibrary } from "./EmptyLibrary";
import { useLibraryStore } from "@/stores/library-store";

export function SongList() {
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
      className="custom-scrollbar flex-1 space-y-1 overflow-y-auto"
      data-song-list-visual-variant="unified"
    >
      {filteredSongs.map((song) => (
        <SongListItem
          key={song.hash}
          song={song}
          orderedHashes={orderedHashes}
        />
      ))}
    </div>
  );
}
