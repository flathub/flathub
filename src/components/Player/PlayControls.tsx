import { Play, Pause, SkipBack, SkipForward } from "lucide-react";
import { usePlayerStore } from "@/stores/player-store";

export function PlayControls() {
  const snapshot = usePlayerStore((s) => s.snapshot);
  const playSong = usePlayerStore((s) => s.playSong);
  const pause = usePlayerStore((s) => s.pause);
  const skipBack = usePlayerStore((s) => s.skipBack);
  const skipForward = usePlayerStore((s) => s.skipForward);
  const isPlaying = snapshot?.is_playing ?? false;

  const handleToggle = () => {
    if (isPlaying) {
      pause();
    } else if (snapshot?.song_id) {
      playSong(snapshot.song_id);
    }
  };

  return (
    <div className="flex items-center gap-4 text-[#EBEBF5]">
      <button onClick={skipBack} className="opacity-80 transition-colors hover:text-white hover:opacity-100" title="Previous" aria-label="Previous">
        <SkipBack size={20} fill="currentColor" />
      </button>
      <button
        onClick={handleToggle}
        className="flex h-8 w-8 items-center justify-center rounded-full bg-[#EBEBF5] text-[var(--color-surface)] transition-transform hover:scale-105"
        title={isPlaying ? "Pause" : "Play"}
        aria-label={isPlaying ? "Pause" : "Play"}
      >
        {isPlaying ? (
          <Pause size={16} fill="currentColor" />
        ) : (
          <Play size={16} fill="currentColor" className="ml-0.5" />
        )}
      </button>
      <button onClick={skipForward} className="opacity-80 transition-colors hover:text-white hover:opacity-100" title="Next" aria-label="Next">
        <SkipForward size={20} fill="currentColor" />
      </button>
    </div>
  );
}
