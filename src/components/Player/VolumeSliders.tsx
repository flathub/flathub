import { useState, useCallback, useMemo, useRef, useEffect } from "react";
import { useTranslation } from "react-i18next";
import {
  Mic2,
  Music,
  ChevronDown,
  Drum,
  Guitar,
  AudioWaveform,
} from "lucide-react";
import { Tooltip } from "@/components/Overlay/Tooltip";
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
  const prevDrumsRef = useRef(1);
  const prevBassRef = useRef(1);
  const prevOtherRef = useRef(1);

  // Click-outside to close popup
  const popupRef = useRef<HTMLDivElement>(null);
  const chevronRef = useRef<HTMLButtonElement>(null);

  useEffect(() => {
    if (!isExpanded) return;
    const handleClickOutside = (e: MouseEvent) => {
      if (
        popupRef.current &&
        !popupRef.current.contains(e.target as Node) &&
        chevronRef.current &&
        !chevronRef.current.contains(e.target as Node)
      ) {
        setIsExpanded(false);
      }
    };
    document.addEventListener("mousedown", handleClickOutside);
    return () => document.removeEventListener("mousedown", handleClickOutside);
  }, [isExpanded]);

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

  const handleDrumsMuteToggle = useCallback(() => {
    if (stemVolumes.drums > 0) {
      prevDrumsRef.current = stemVolumes.drums;
      setStemVolume("drums", 0);
    } else {
      setStemVolume("drums", prevDrumsRef.current);
    }
  }, [stemVolumes.drums, setStemVolume]);

  const handleBassMuteToggle = useCallback(() => {
    if (stemVolumes.bass > 0) {
      prevBassRef.current = stemVolumes.bass;
      setStemVolume("bass", 0);
    } else {
      setStemVolume("bass", prevBassRef.current);
    }
  }, [stemVolumes.bass, setStemVolume]);

  const handleOtherMuteToggle = useCallback(() => {
    if (stemVolumes.other > 0) {
      prevOtherRef.current = stemVolumes.other;
      setStemVolume("other", 0);
    } else {
      setStemVolume("other", prevOtherRef.current);
    }
  }, [stemVolumes.other, setStemVolume]);

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

      {/* Accompaniment group — relative for popup anchor */}
      <div className="relative flex items-center gap-2">
        <StemSlider
          icon={<Music size={14} />}
          label={t("stems.accompaniment")}
          value={accompValue}
          onChange={handleAccompChange}
          onIconClick={stemsAvailable ? handleAccompMuteToggle : undefined}
          disabled={!stemsAvailable}
        />
        {stemsAvailable && isFourStem && (
          <Tooltip
            label={
              isExpanded ? t("stems.collapseStems") : t("stems.expandStems")
            }
          >
            <button
              ref={chevronRef}
              onClick={() => setIsExpanded(!isExpanded)}
              aria-label={
                isExpanded ? t("stems.collapseStems") : t("stems.expandStems")
              }
              className="motion-icon-button flex h-4 w-4 items-center justify-center rounded-full text-[var(--color-text-dimmer)] hover:bg-white/4 hover:text-[#EBEBF5] focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--color-accent)]/30"
            >
              <ChevronDown
                size={12}
                className={`transition-transform ${isExpanded ? "rotate-180" : ""}`}
              />
            </button>
          </Tooltip>
        )}

        {/* Popup for individual stem controls — aligned with accompaniment */}
        {isExpanded && stemsAvailable && isFourStem && (
          <div
            ref={popupRef}
            className="absolute bottom-full left-0 z-50 mb-3 rounded-lg border border-[color-mix(in_srgb,var(--color-border)_85%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_90%,transparent)] p-3 shadow-[0_20px_40px_rgba(0,0,0,0.32)] backdrop-blur-xl animate-[song-fade-in_var(--motion-duration-standard)_var(--motion-ease-emphasized-out)]"
          >
            <div className="flex flex-col gap-2">
              <StemSlider
                icon={<Drum size={13} />}
                label={t("stems.drums")}
                value={stemVolumes.drums}
                onChange={(v) => handleStemChange("drums", v)}
                onIconClick={handleDrumsMuteToggle}
              />
              <StemSlider
                icon={<Guitar size={13} />}
                label={t("stems.bass")}
                value={stemVolumes.bass}
                onChange={(v) => handleStemChange("bass", v)}
                onIconClick={handleBassMuteToggle}
              />
              <StemSlider
                icon={<AudioWaveform size={13} />}
                label={t("stems.other")}
                value={stemVolumes.other}
                onChange={(v) => handleStemChange("other", v)}
                onIconClick={handleOtherMuteToggle}
              />
            </div>
          </div>
        )}
      </div>
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
  const muteLabel =
    value === 0
      ? t("stems.unmute", { stem: label })
      : t("stems.mute", { stem: label });

  return (
    <div className="flex items-center gap-2">
      <Tooltip label={onIconClick ? muteLabel : label}>
        <button
          onClick={onIconClick}
          disabled={disabled || !onIconClick}
          className={`motion-icon-button rounded-full p-1 ${
            !disabled && value > 0
              ? "text-[#EBEBF5] hover:bg-white/4 hover:text-white"
              : "text-[var(--color-text-dimmer)]"
          } ${onIconClick && !disabled ? "cursor-pointer" : "cursor-default"}`}
          aria-label={onIconClick ? muteLabel : label}
        >
          {icon}
        </button>
      </Tooltip>
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
