import { describe, expect, test, vi } from "vitest";
import { loadStartupSettings } from "./settings-runtime";

describe("unified app runtime module", () => {
  test("exports a single gated hook graph with no sidebar webview fork", async () => {
    const { default: src } = await import("./app-runtime.ts?raw");

    expect(src).toContain("export function useAppRuntime");
    expect(src).not.toContain("useSidebarWindowRuntimeWhen");
    expect(src).not.toContain("useSidebarPaneEventListeners");
    expect(src).not.toContain("sidebar-webview");
    expect(src).toContain("useEventListeners");
    expect(src).toContain("useLyricsAutoFetch");
    expect(src).toContain("useKeyboardShortcuts");
    expect(src).toContain("useFileDrop");
    expect(src).toContain("useAppMenuRuntime");
  });
});

describe("app runtime settings hydration", () => {
  test("hydrates settings and applies the persisted language", async () => {
    const getSettings = vi.fn().mockResolvedValue({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
      lyrics_font_step: 1,
    });
    const hydrateAppSettings = vi.fn();
    const changeLanguage = vi.fn().mockResolvedValue(undefined);
    const detectFallbackLanguage = vi.fn(() => "en");

    await loadStartupSettings({
      getSettings,
      hydrateAppSettings,
      changeLanguage,
      detectFallbackLanguage,
    });

    expect(hydrateAppSettings).toHaveBeenCalledWith({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
      lyrics_font_step: 1,
    });
    expect(changeLanguage).toHaveBeenCalledWith("zh-CN");
    expect(detectFallbackLanguage).not.toHaveBeenCalled();
  });

  test("falls back to the detected system language when none is saved", async () => {
    const getSettings = vi.fn().mockResolvedValue({
      stem_mode: "two_stem",
      model_variant: "htdemucs",
      language: null,
      hide_batch_separate: false,
      lyrics_font_step: 0,
      execution_provider: "xnnpack",
      available_execution_providers: ["cpu", "xnnpack"],
    });
    const hydrateAppSettings = vi.fn();
    const changeLanguage = vi.fn().mockResolvedValue(undefined);
    const detectFallbackLanguage = vi.fn(() => "ja");

    await loadStartupSettings({
      getSettings,
      hydrateAppSettings,
      changeLanguage,
      detectFallbackLanguage,
    });

    expect(hydrateAppSettings).toHaveBeenCalledOnce();
    expect(detectFallbackLanguage).toHaveBeenCalledOnce();
    expect(changeLanguage).toHaveBeenCalledWith("ja");
  });
});
