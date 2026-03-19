import { useEffect, useState } from "react";
import { Tooltip } from "@/components/Overlay/Tooltip";

interface AudioLevelSliderProps {
  label: string;
  value: number;
  onChange: (value: number) => void;
  disabled?: boolean;
  widthClass?: string;
  ariaLabel?: string;
}

function formatAudioLevelTooltip(label: string, value: number): string {
  return `${label} ${Math.round(value * 100)}%`;
}

export function AudioLevelSlider({
  label,
  value,
  onChange,
  disabled = false,
  widthClass = "w-16",
  ariaLabel,
}: AudioLevelSliderProps) {
  const [isDragging, setIsDragging] = useState(false);

  useEffect(() => {
    if (!isDragging) {
      return;
    }

    const handlePointerFinish = () => {
      setIsDragging(false);
    };

    window.addEventListener("pointerup", handlePointerFinish);
    window.addEventListener("pointercancel", handlePointerFinish);

    return () => {
      window.removeEventListener("pointerup", handlePointerFinish);
      window.removeEventListener("pointercancel", handlePointerFinish);
    };
  }, [isDragging]);

  // Keep the native range input so we preserve platform drag semantics and only
  // layer immediate tooltip/highlight behavior on top.
  return (
    <Tooltip label={formatAudioLevelTooltip(label, value)}>
      <input
        type="range"
        min="0"
        max="100"
        value={Math.round(value * 100)}
        onChange={(e) => onChange(Number(e.target.value) / 100)}
        onPointerDown={() => setIsDragging(true)}
        onBlur={() => setIsDragging(false)}
        className={`native-slider audio-level-slider ${widthClass}`}
        disabled={disabled}
        data-dragging={isDragging ? "true" : undefined}
        aria-label={ariaLabel ?? label}
      />
    </Tooltip>
  );
}
