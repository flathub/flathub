import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { listen } from "@tauri-apps/api/event";
import { useTranslation } from "react-i18next";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import { syncAirPlayRoutePicker } from "@/lib/tauri";
import type {
  AirPlayOutputStateEvent,
  AirPlayRoutePickerBounds,
} from "@/types/ipc";

const AIRPLAY_OUTPUT_STATE_EVENT = "openkara://airplay-output-state";

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
  className = "h-9 w-9 rounded-xl",
}: AirPlayRouteButtonProps) {
  const { t } = useTranslation();
  const platform = getShortcutPlatform();
  const hostRef = useRef<HTMLDivElement>(null);
  const [isActive, setIsActive] = useState(false);

  useEffect(() => {
    if (platform !== "mac") {
      return;
    }

    let cancelled = false;
    let unlisten: (() => void) | null = null;

    const setup = async () => {
      unlisten = await listen<AirPlayOutputStateEvent>(
        AIRPLAY_OUTPUT_STATE_EVENT,
        (event) => {
          if (!cancelled) {
            setIsActive(event.payload.active);
          }
        },
      );
    };

    void setup();

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, [platform]);

  useLayoutEffect(() => {
    if (platform !== "mac" || !hostRef.current) {
      return;
    }

    const hostEl = hostRef.current;

    const syncBounds = () => {
      const host = hostRef.current;
      if (!host) {
        return;
      }

      void syncAirPlayRoutePicker(buildHostBounds(host)).catch((err) => {
        // RATIONALE: The native control is auxiliary to local playback. Transient
        // mount/update failures must not surface as blocking UI; log so operators
        // can diagnose without leaving the toolbar unresponsive.
        console.error("syncAirPlayRoutePicker failed:", err);
      });
    };

    syncBounds();
    window.addEventListener("resize", syncBounds);

    const resizeObserver = new ResizeObserver(() => {
      syncBounds();
    });
    resizeObserver.observe(hostEl);

    return () => {
      window.removeEventListener("resize", syncBounds);
      resizeObserver.disconnect();
      void syncAirPlayRoutePicker(null).catch((err) => {
        // RATIONALE: Teardown is best-effort; a failed unpick should not prevent
        // unmount or obscure the underlying issue in devtools.
        console.error("syncAirPlayRoutePicker teardown failed:", err);
      });
    };
  }, [platform]);

  if (platform !== "mac") {
    return null;
  }

  return (
    <Tooltip label={t("player.airPlayOutput")}>
      <div
        className={`relative border transition-colors ${className} ${
          isActive
            ? "border-[color-mix(in_srgb,var(--color-accent)_42%,var(--color-border))] bg-[color-mix(in_srgb,var(--color-accent)_12%,transparent)] shadow-[0_10px_24px_rgba(0,0,0,0.16)]"
            : "border-transparent bg-transparent"
        }`}
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
