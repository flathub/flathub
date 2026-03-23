import { beforeEach, describe, expect, test, vi } from "vitest";

const { mockInvoke } = vi.hoisted(() => ({
  mockInvoke: vi.fn(),
}));

vi.mock("@tauri-apps/api/core", () => ({
  invoke: mockInvoke,
}));

import { getCdgFrame } from "./tauri";

describe("tauri API wrappers", () => {
  beforeEach(() => {
    mockInvoke.mockReset();
    mockInvoke.mockResolvedValue(null);
  });

  test("sends the backend positionMs payload name", async () => {
    await getCdgFrame(123.6);

    expect(mockInvoke).toHaveBeenCalledWith("get_cdg_frame", {
      positionMs: 124,
    });
  });

  test("syncs the native AirPlay route picker bounds", async () => {
    const { syncAirPlayRoutePicker } = await import("./tauri");

    await syncAirPlayRoutePicker({
      left: 12,
      top: 34,
      width: 140,
      height: 28,
    });

    expect(mockInvoke).toHaveBeenCalledWith("sync_airplay_route_picker", {
      bounds: {
        left: 12,
        top: 34,
        width: 140,
        height: 28,
      },
    });
  });

  test("expands import paths through the dedicated library command", async () => {
    const { expandImportPaths } = await import("./tauri");

    await expandImportPaths(["/music/library"]);

    expect(mockInvoke).toHaveBeenCalledWith("expand_import_paths", {
      paths: ["/music/library"],
    });
  });

  test("opens the mixed import picker through the dedicated backend command", async () => {
    const { pickImportPaths } = await import("./tauri");

    await pickImportPaths("/music");

    expect(mockInvoke).toHaveBeenCalledWith("pick_import_paths", {
      defaultPath: "/music",
    });
  });

  test("passes a null default path to the mixed import picker when none is available", async () => {
    const { pickImportPaths } = await import("./tauri");

    await pickImportPaths();

    expect(mockInvoke).toHaveBeenCalledWith("pick_import_paths", {
      defaultPath: null,
    });
  });

  test("syncs audience state to the AirPlay backend", async () => {
    const { syncAirPlayAudienceState } = await import("./tauri");

    await syncAirPlayAudienceState({
      mode: "lyrics",
      songId: "song-1",
      lines: [{ time_ms: 3000, text: "Line", words: null }],
      offsetMs: 100,
      isLoading: false,
      lyricsFontStep: 1,
      messages: {
        selectSong: "Select a song to start",
        loadingLyrics: "Loading lyrics...",
        noLyrics: "No lyrics available for this track",
        addLyrics: "Add Lyrics",
      },
      viewport: {
        widthPx: 1280,
        heightPx: 720,
        bottomInsetPx: 0,
      },
      presentationSpec: {
        contentWidthRatio: 0.92,
        contentMaxWidthPx: 1600,
        horizontalPaddingPx: 64,
        verticalPaddingPx: 56,
        lineGapPx: 40,
        fontSizePx: 96,
        lineHeightMultiple: 1.08,
        activeScale: 1.05,
        statusFontSizePx: 18,
        activeGlowBlurPx: 12,
        activeTextColor: { red: 1, green: 1, blue: 1, alpha: 1 },
        pastTextColor: {
          red: 72 / 255,
          green: 72 / 255,
          blue: 74 / 255,
          alpha: 1,
        },
        futureTextColor: {
          red: 58 / 255,
          green: 58 / 255,
          blue: 60 / 255,
          alpha: 1,
        },
        plainTextColor: { red: 1, green: 1, blue: 1, alpha: 1 },
        statusTextColor: {
          red: 142 / 255,
          green: 142 / 255,
          blue: 147 / 255,
          alpha: 1,
        },
        activeGlowColor: { red: 1, green: 1, blue: 1, alpha: 0.8 },
      },
    });

    expect(mockInvoke).toHaveBeenCalledWith("sync_airplay_audience_state", {
      payload: {
        mode: "lyrics",
        songId: "song-1",
        lines: [{ time_ms: 3000, text: "Line", words: null }],
        offsetMs: 100,
        isLoading: false,
        lyricsFontStep: 1,
        messages: {
          selectSong: "Select a song to start",
          loadingLyrics: "Loading lyrics...",
          noLyrics: "No lyrics available for this track",
          addLyrics: "Add Lyrics",
        },
        viewport: {
          widthPx: 1280,
          heightPx: 720,
          bottomInsetPx: 0,
        },
        presentationSpec: {
          contentWidthRatio: 0.92,
          contentMaxWidthPx: 1600,
          horizontalPaddingPx: 64,
          verticalPaddingPx: 56,
          lineGapPx: 40,
          fontSizePx: 96,
          lineHeightMultiple: 1.08,
          activeScale: 1.05,
          statusFontSizePx: 18,
          activeGlowBlurPx: 12,
          activeTextColor: { red: 1, green: 1, blue: 1, alpha: 1 },
          pastTextColor: {
            red: 72 / 255,
            green: 72 / 255,
            blue: 74 / 255,
            alpha: 1,
          },
          futureTextColor: {
            red: 58 / 255,
            green: 58 / 255,
            blue: 60 / 255,
            alpha: 1,
          },
          plainTextColor: { red: 1, green: 1, blue: 1, alpha: 1 },
          statusTextColor: {
            red: 142 / 255,
            green: 142 / 255,
            blue: 147 / 255,
            alpha: 1,
          },
          activeGlowColor: { red: 1, green: 1, blue: 1, alpha: 0.8 },
        },
      },
    });
  });

  test("steps AirPlay plain-text pages through the dedicated command", async () => {
    const { stepAirPlayPlainTextPage } = await import("./tauri");

    await stepAirPlayPlainTextPage("next");

    expect(mockInvoke).toHaveBeenCalledWith("step_airplay_plain_text_page", {
      direction: "next",
    });
  });

  test("reads the native window shell snapshot through the dedicated command", async () => {
    const { getWindowShellState } = await import("./tauri");

    await getWindowShellState();

    expect(mockInvoke).toHaveBeenCalledWith("get_window_shell_state");
  });

  test("syncs native sidebar visibility through the dedicated shell command", async () => {
    const { setNativeSidebarVisibility } = await import("./tauri");

    await setNativeSidebarVisibility(false);

    expect(mockInvoke).toHaveBeenCalledWith("set_native_sidebar_visibility", {
      visible: false,
    });
  });

  test("persists the macOS shell mode through the dedicated settings command", async () => {
    const { setMacOsShellMode } = await import("./tauri");

    await setMacOsShellMode("native");

    expect(mockInvoke).toHaveBeenCalledWith("set_macos_shell_mode", {
      mode: "native",
    });
  });

  test("requests an app restart through the dedicated lifecycle command", async () => {
    const { restartApp } = await import("./tauri");

    await restartApp();

    expect(mockInvoke).toHaveBeenCalledWith("restart_app");
  });
});
