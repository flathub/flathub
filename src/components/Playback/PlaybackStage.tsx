import { LyricsPanel } from "@/components/Lyrics/LyricsPanel";
import { CdgCanvas } from "@/components/Cdg/CdgCanvas";
import { useCdgStore } from "@/stores/cdg-store";

export function PlaybackStage() {
  const hasCdg = useCdgStore((s) => s.hasCdg);
  return hasCdg ? <CdgCanvas /> : <LyricsPanel />;
}
