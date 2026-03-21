import { emit, emitTo } from "@tauri-apps/api/event";
import { stepAirPlayPlainTextPage } from "@/lib/tauri";
import { usePlayerStore } from "@/stores/player-store";
import type { AirPlayOutputStateEvent } from "@/types/ipc";

export type PlainTextPageDirection = "prev" | "next";
export type PlainTextRemoteTarget = "airplay" | "local";

export const LOCAL_AUDIENCE_OUTPUT_STATE_EVENT =
  "openkara://local-audience-output-state";
export const LOCAL_AUDIENCE_PLAIN_TEXT_PAGE_EVENT =
  "openkara://local-audience-plain-text-page";

export function getAirPlayPlainTextPageLockMs(
  latencyMs: number | null,
): number {
  return Math.max(900, Math.min((latencyMs ?? 1200) + 250, 2500));
}

export function resolvePlainTextRemoteTarget(
  airPlayOutput: Pick<AirPlayOutputStateEvent, "active" | "phase">,
  localAudienceOutputActive: boolean,
): PlainTextRemoteTarget | null {
  if (airPlayOutput.active) {
    return "airplay";
  }

  return localAudienceOutputActive ? "local" : null;
}

export async function announceLocalAudienceOutputActive(
  active: boolean,
): Promise<void> {
  await emit(LOCAL_AUDIENCE_OUTPUT_STATE_EVENT, { active });
}

export async function stepPlainTextRemotePage(
  airPlayOutput: Pick<AirPlayOutputStateEvent, "active" | "phase"> & {
    latencyMs?: number | null;
  },
  localAudienceOutputActive: boolean,
  direction: PlainTextPageDirection,
): Promise<boolean> {
  const target = resolvePlainTextRemoteTarget(
    airPlayOutput,
    localAudienceOutputActive,
  );

  if (!target) {
    return false;
  }

  if (target === "airplay") {
    if (usePlayerStore.getState().airPlayPlainTextPagePending) {
      return false;
    }

    await stepAirPlayPlainTextPage(direction);
    usePlayerStore
      .getState()
      .startAirPlayPlainTextPagePending(
        direction,
        getAirPlayPlainTextPageLockMs(airPlayOutput.latencyMs ?? null),
      );
  } else {
    await emitTo("fullscreen-player", LOCAL_AUDIENCE_PLAIN_TEXT_PAGE_EVENT, {
      direction,
    });
  }

  return true;
}
