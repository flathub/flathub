import { beforeEach, describe, expect, test, vi } from "vitest";

const { mockEmit, mockEmitTo, mockStepAirPlayPlainTextPage } = vi.hoisted(
  () => ({
    mockEmit: vi.fn(),
    mockEmitTo: vi.fn(),
    mockStepAirPlayPlainTextPage: vi.fn(),
  }),
);

vi.mock("@tauri-apps/api/event", () => ({
  emit: mockEmit,
  emitTo: mockEmitTo,
}));

vi.mock("@/lib/tauri", () => ({
  stepAirPlayPlainTextPage: mockStepAirPlayPlainTextPage,
}));

import {
  announceLocalAudienceOutputActive,
  resolvePlainTextRemoteTarget,
  stepPlainTextRemotePage,
} from "./plain-text-page-controls";

describe("plain-text page controls", () => {
  beforeEach(() => {
    mockEmit.mockReset();
    mockEmitTo.mockReset();
    mockStepAirPlayPlainTextPage.mockReset();
    mockEmit.mockResolvedValue(undefined);
    mockEmitTo.mockResolvedValue(undefined);
    mockStepAirPlayPlainTextPage.mockResolvedValue(undefined);
  });

  test("prefers AirPlay over the local audience window when both are available", () => {
    expect(
      resolvePlainTextRemoteTarget({ active: true, phase: "playing" }, true),
    ).toBe("airplay");
    expect(
      resolvePlainTextRemoteTarget({ active: false, phase: "playing" }, true),
    ).toBe("local");
    expect(
      resolvePlainTextRemoteTarget({ active: false, phase: "idle" }, false),
    ).toBeNull();
  });

  test("routes page steps to AirPlay when AirPlay is active", async () => {
    await stepPlainTextRemotePage(
      { active: true, phase: "buffering" },
      true,
      "next",
    );

    expect(mockStepAirPlayPlainTextPage).toHaveBeenCalledWith("next");
    expect(mockEmitTo).not.toHaveBeenCalled();
  });

  test("routes page steps to the fullscreen player when only the local audience is active", async () => {
    await stepPlainTextRemotePage(
      { active: false, phase: "idle" },
      true,
      "prev",
    );

    expect(mockEmitTo).toHaveBeenCalledWith(
      "fullscreen-player",
      "openkara://local-audience-plain-text-page",
      { direction: "prev" },
    );
    expect(mockStepAirPlayPlainTextPage).not.toHaveBeenCalled();
  });

  test("broadcasts local audience active state updates", async () => {
    await announceLocalAudienceOutputActive(true);

    expect(mockEmit).toHaveBeenCalledWith(
      "openkara://local-audience-output-state",
      { active: true },
    );
  });
});
