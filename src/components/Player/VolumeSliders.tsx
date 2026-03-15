import { useState, useCallback, useMemo } from "react";
import { Mic2, Music, ChevronDown, Drum, Guitar, Piano } from "lucide-react";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import type { StemName } from "@/types/ipc";

export function VolumeSliders() {
  const snapshot = usePlayerStore((s) => s.snapshot);
  const setStemVolume = usePlayerStore((s) => s.setStemVolume);
  const separationStatuses = useLibraryStore((s) => s.separationStatuses);

  const [isExpanded, setIsExpanded] = useState(false);

  const stemVolumes = useMemo(
    () =>
      snapshot?.stem_volumes ?? {
        vocals: 1,
        drums: 1,
        bass: 1,
        other: 1,
      },
    [snapshot?.stem_volumes],
  );
  const hasStems = snapshot?.has_stems ?? false;
  const stemMode = snapshot?.stem_mode ?? null;
  const songId = snapshot?.song_id;
  const isSeparated =
    songId != null && separationStatuses[songId]?.state === "completed";
  const stemsAvailable = hasStems && isSeparated;
  const isTwoStem = stemMode === "two_stem";
  const isFourStem = stemMode === "four_stem";

  const handleStemChange = useCallback(
    (stem: StemName, value: number) => {
      setStemVolume(stem, value);
    },
    [setStemVolume],
  );

  // Accompaniment display value = max of the three sub-stems
  const accompValue = Math.max(
    stemVolumes.drums,
    stemVolumes.bass,
    stemVolumes.other,
  );

  const handleAccompChange = useCallback(
    (newValue: number) => {
      if (isTwoStem) {
        // In 2-stem mode, set all three sub-stems to the same value;
        // the backend uses drums gain as the accompaniment gain.
        setStemVolume("drums", newValue);
        setStemVolume("bass", newValue);
        setStemVolume("other", newValue);
      } else if (accompValue === 0) {
        // All sub-stems are 0; set them all to the new value
        setStemVolume("drums", newValue);
        setStemVolume("bass", newValue);
        setStemVolume("other", newValue);
      } else {
        const ratio = newValue / accompValue;
        setStemVolume(
          "drums",
          Math.min(1, stemVolumes.drums * ratio),
        );
        setStemVolume(
          "bass",
          Math.min(1, stemVolumes.bass * ratio),
        );
        setStemVolume(
          "other",
          Math.min(1, stemVolumes.other * ratio),
        );
      }
    },
    [isTwoStem, accompValue, stemVolumes, setStemVolume],
  );

  return (
    <div className="flex items-center gap-5">
      {/* Vocals slider */}
      <StemSlider
        icon={<Mic2 size={14} />}
        label="Vocals"
        value={stemVolumes.vocals}
        onChange={(v) => handleStemChange("vocals", v)}
        disabled={!stemsAvailable}
      />

      {/* Accompaniment group */}
      <div className="flex items-center gap-2">
        <StemSlider
          icon={<Music size={14} />}
          label="Accompaniment"
          value={accompValue}
          onChange={handleAccompChange}
          disabled={!stemsAvailable}
        />
        {stemsAvailable && isFourStem && (
          <button
            onClick={() => setIsExpanded(!isExpanded)}
            className="flex items-center justify-center w-4 h-4 text-[var(--color-text-dimmer)] hover:text-[#EBEBF5] transition-colors"
            title={isExpanded ? "Collapse stems" : "Expand stems"}
          >
            <ChevronDown
              size={12}
              className={`transition-transform ${isExpanded ? "rotate-180" : ""}`}
            />
          </button>
        )}
      </div>

      {/* Expanded individual stem sliders (4-stem mode only) */}
      {isExpanded && stemsAvailable && isFourStem && (
        <>
          <StemSlider
            icon={<Drum size={13} />}
            label="Drums"
            value={stemVolumes.drums}
            onChange={(v) => handleStemChange("drums", v)}
          />
          <StemSlider
            icon={<Guitar size={13} />}
            label="Bass"
            value={stemVolumes.bass}
            onChange={(v) => handleStemChange("bass", v)}
          />
          <StemSlider
            icon={<Piano size={13} />}
            label="Other"
            value={stemVolumes.other}
            onChange={(v) => handleStemChange("other", v)}
          />
        </>
      )}
    </div>
  );
}

function StemSlider({
  icon,
  label,
  value,
  onChange,
  disabled = false,
}: {
  icon: React.ReactNode;
  label: string;
  value: number;
  onChange: (value: number) => void;
  disabled?: boolean;
}) {
  return (
    <div className="flex items-center gap-2" title={label}>
      <span
        className={
          !disabled && value > 0
            ? "text-[#EBEBF5]"
            : "text-[var(--color-text-dimmer)]"
        }
      >
        {icon}
      </span>
      <input
        type="range"
        min="0"
        max="100"
        value={Math.round(value * 100)}
        onChange={(e) => onChange(Number(e.target.value) / 100)}
        className="native-slider w-16"
        disabled={disabled}
      />
    </div>
  );
}
