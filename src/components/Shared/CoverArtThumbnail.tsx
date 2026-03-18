import { useState } from "react";
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
  const [failedUrl, setFailedUrl] = useState<string | null>(null);

  return (
    <div
      className={`overflow-hidden rounded-[10px] border border-[color-mix(in_srgb,var(--color-border)_82%,transparent)] bg-[linear-gradient(180deg,rgba(255,255,255,0.08),rgba(255,255,255,0.03))] ${className}`}
    >
      {/* These covers already live in the local database. In desktop WebViews,
          lazy/async image decoding can leave blob-backed thumbnails unpainted,
          so we render them eagerly and fall back immediately on load failure. */}
      {url && failedUrl !== url ? (
        <img
          src={url}
          alt={alt}
          onError={() => setFailedUrl(url)}
          className="block h-full w-full object-cover"
        />
      ) : (
        <div className="flex h-full w-full items-center justify-center bg-[radial-gradient(circle_at_top,rgba(255,255,255,0.18),rgba(255,255,255,0.04)_58%,rgba(0,0,0,0.12))]">
          <span className="h-2.5 w-2.5 rounded-full bg-white/28" aria-hidden />
        </div>
      )}
    </div>
  );
}
