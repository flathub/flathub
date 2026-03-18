import { useCoverArtUrl } from "@/lib/cover-art";
import type { CoverArtBytes } from "@/types/ipc";

interface CoverArtThumbnailProps {
  songHash: string;
  coverArt: CoverArtBytes;
  alt: string;
  className?: string;
}

export function CoverArtThumbnail({
  songHash,
  coverArt,
  alt,
  className = "",
}: CoverArtThumbnailProps) {
  const url = useCoverArtUrl(songHash, coverArt);

  return (
    <div
      className={`overflow-hidden rounded-[10px] border border-[color-mix(in_srgb,var(--color-border)_82%,transparent)] bg-[linear-gradient(180deg,rgba(255,255,255,0.08),rgba(255,255,255,0.03))] ${className}`}
    >
      {url ? (
        <img
          src={url}
          alt={alt}
          loading="lazy"
          decoding="async"
          className="h-full w-full object-cover"
        />
      ) : (
        <div className="flex h-full w-full items-center justify-center bg-[radial-gradient(circle_at_top,rgba(255,255,255,0.18),rgba(255,255,255,0.04)_58%,rgba(0,0,0,0.12))]">
          <span className="h-2.5 w-2.5 rounded-full bg-white/28" aria-hidden />
        </div>
      )}
    </div>
  );
}
