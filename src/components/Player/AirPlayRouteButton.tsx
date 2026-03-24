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

    const host = hostRef.current;

    const syncBounds = () => {
      if (!hostRef.current) {
        return;
      }

      void syncAirPlayRoutePicker(buildHostBounds(hostRef.current)).catch(
        () => {
          // The native control is auxiliary to local playback. If mount/update
          // fails transiently, keep the toolbar responsive instead of surfacing
          // a blocking error.
        },
      );
    };

    syncBounds();
    window.addEventListener("resize", syncBounds);

    // RATIONALE: The native AVRoutePickerView is mounted outside the DOM. When
    // the floating shell reflows without a window resize, we must re-publish the
    // host bounds or the visible click target drifts away from the toolbar slot.
    const resizeObserver =
      typeof ResizeObserver === "undefined"
        ? null
        : new ResizeObserver(syncBounds);
    resizeObserver?.observe(host);

    return () => {
      window.removeEventListener("resize", syncBounds);
      resizeObserver?.disconnect();
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
