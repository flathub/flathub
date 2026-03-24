import { useLayoutEffect, useRef } from "react";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { syncAirPlayRoutePicker } from "@/lib/tauri";
import type { AirPlayRoutePickerBounds } from "@/types/ipc";

function buildHostBounds(element: HTMLDivElement): AirPlayRoutePickerBounds {
  const rect = element.getBoundingClientRect();
  return {
    left: rect.left,
    top: rect.top,
    width: rect.width,
    height: rect.height,
  };
}

interface AirPlayRouteButtonProps {
  className?: string;
}

export function AirPlayRouteButton({
  className = "h-8 w-8 flex items-center justify-center",
}: AirPlayRouteButtonProps) {
  const { t } = useTranslation();
  const platform = getShortcutPlatform();
  const hostRef = useRef<HTMLDivElement>(null);

  useLayoutEffect(() => {
    if (platform !== "mac" || !hostRef.current) {
      return;
    }

    const syncBounds = () => {
      const host = hostRef.current;
      if (!host) {
        return;
      }

      void syncAirPlayRoutePicker(buildHostBounds(host)).catch(() => {
        // The native control is auxiliary to local playback. If mount/update
        // fails transiently, keep the toolbar responsive instead of surfacing
        // a blocking error.
      });
    };

    syncBounds();
    window.addEventListener("resize", syncBounds);

    return () => {
      window.removeEventListener("resize", syncBounds);
      void syncAirPlayRoutePicker(null).catch(() => {
        // Best effort teardown only.
      });
    };
  }, [platform]);

  if (platform !== "mac") {
    return null;
  }

  return (
    <Tooltip label={t("player.airPlayOutput")}>
      <div
        className={`relative ${className}`}
        data-airplay-route-button="true"
        aria-label={t("player.airPlayOutput")}
      >
        <div
          ref={hostRef}
          className="h-full w-full rounded-[inherit]"
          data-airplay-route-host="true"
        />
      </div>
    </Tooltip>
  );
}
