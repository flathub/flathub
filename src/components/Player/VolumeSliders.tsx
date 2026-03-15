import { useState, useCallback, useMemo, useRef } from "react";
import { useTranslation } from "react-i18next";
import {
  Mic2,
  Music,
  ChevronDown,
  Drum,
  Guitar,
  Piano,
} from "lucide-react";
import { usePlayerStore } from "@/stores/player-store";
import { useLibraryStore } from "@/stores/library-store";
import type { StemName } from "@/types/ipc";

export function VolumeSliders() {
  const { t } = useTranslation();
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

  // Track previous non-zero values for mute/unmute toggle
  const prevVocalsRef = useRef(1);
  const prevAccompRef = useRef(1);

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
        // the backend uses max gain as the accompaniment gain.
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
        setStemVolume("drums", Math.min(1, stemVolumes.drums * ratio));
        setStemVolume("bass", Math.min(1, stemVolumes.bass * ratio));
        setStemVolume("other", Math.min(1, stemVolumes.other * ratio));
      }
    },
    [isTwoStem, accompValue, stemVolumes, setStemVolume],
  );

  const handleVocalsMuteToggle = useCallback(() => {
    if (stemVolumes.vocals > 0) {
      prevVocalsRef.current = stemVolumes.vocals;
      setStemVolume("vocals", 0);
    } else {
      setStemVolume("vocals", prevVocalsRef.current);
    }
  }, [stemVolumes.vocals, setStemVolume]);

  const handleAccompMuteToggle = useCallback(() => {
    if (accompValue > 0) {
      prevAccompRef.current = accompValue;
      setStemVolume("drums", 0);
      setStemVolume("bass", 0);
      setStemVolume("other", 0);
    } else {
      const prev = prevAccompRef.current;
      setStemVolume("drums", prev);
      setStemVolume("bass", prev);
      setStemVolume("other", prev);
    }
  }, [accompValue, setStemVolume]);

  return (
    <div className="flex items-center gap-5">
      {/* Vocals slider */}
      <StemSlider
        icon={<Mic2 size={14} />}
        label={t("stems.vocals")}
        value={stemVolumes.vocals}
        onChange={(v) => handleStemChange("vocals", v)}
        onIconClick={stemsAvailable ? handleVocalsMuteToggle : undefined}
        disabled={!stemsAvailable}
      />

      {/* Accompaniment group */}
      <div className="flex items-center gap-2">
        <StemSlider
          icon={<Music size={14} />}
          label={t("stems.accompaniment")}
          value={accompValue}
          onChange={handleAccompChange}
          onIconClick={stemsAvailable ? handleAccompMuteToggle : undefined}
          disabled={!stemsAvailable}
        />
        {stemsAvailable && isFourStem && (
          <button
            onClick={() => setIsExpanded(!isExpanded)}
            className="flex h-4 w-4 items-center justify-center text-[var(--color-text-dimmer)] transition-colors hover:text-[#EBEBF5]"
            title={isExpanded ? t("stems.collapseStems") : t("stems.expandStems")}
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
            label={t("stems.drums")}
            value={stemVolumes.drums}
            onChange={(v) => handleStemChange("drums", v)}
          />
          <StemSlider
            icon={<Guitar size={13} />}
            label={t("stems.bass")}
            value={stemVolumes.bass}
            onChange={(v) => handleStemChange("bass", v)}
          />
          <StemSlider
            icon={<Piano size={13} />}
            label={t("stems.other")}
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
  onIconClick,
  disabled = false,
}: {
  icon: React.ReactNode;
  label: string;
  value: number;
  onChange: (value: number) => void;
  onIconClick?: () => void;
  disabled?: boolean;
}) {
  const { t } = useTranslation();
  const muteLabel = value === 0 ? t("stems.unmute", { stem: label }) : t("stems.mute", { stem: label });

  return (
    <div className="flex items-center gap-2">
      <button
        onClick={onIconClick}
        disabled={disabled || !onIconClick}
        className={`transition-colors ${
          !disabled && value > 0
            ? "text-[#EBEBF5] hover:text-white"
            : "text-[var(--color-text-dimmer)]"
        } ${onIconClick && !disabled ? "cursor-pointer" : "cursor-default"}`}
        title={onIconClick ? muteLabel : label}
        aria-label={onIconClick ? muteLabel : label}
      >
        {icon}
      </button>
      <input
        type="range"
        min="0"
        max="100"
        value={Math.round(value * 100)}
        onChange={(e) => onChange(Number(e.target.value) / 100)}
        className="native-slider w-16"
        disabled={disabled}
        title={label}
        aria-label={label}
      />
    </div>
  );
}
