import { beforeEach, describe, expect, test } from "vitest";
import { useSettingsStore } from "./settings-store";

describe("settings-store app settings ownership", () => {
  beforeEach(() => {
    useSettingsStore.setState({
      isOpen: false,
      hydrated: false,
      stemMode: "two_stem",
      modelVariant: "htdemucs",
      language: null,
      hideBatchSeparate: false,
      lyricsFontStep: 0,
    });
  });

  test("hydrateAppSettings stores the full app settings snapshot", () => {
    useSettingsStore.getState().hydrateAppSettings({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
      lyrics_font_step: 1,
    });

    expect(useSettingsStore.getState()).toMatchObject({
      hydrated: true,
      stemMode: "four_stem",
      modelVariant: "htdemucs_ft",
      language: "zh-CN",
      hideBatchSeparate: true,
      lyricsFontStep: 1,
    });
  });

  test("patchAppSettings updates the shared snapshot without resetting other fields", () => {
    useSettingsStore.getState().hydrateAppSettings({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: false,
      lyrics_font_step: -1,
    });

    useSettingsStore.getState().patchAppSettings({
      language: "en",
      hideBatchSeparate: true,
      lyricsFontStep: 2,
    });

    expect(useSettingsStore.getState().getAppSettingsSnapshot()).toEqual({
      hydrated: true,
      stemMode: "four_stem",
      modelVariant: "htdemucs_ft",
      language: "en",
      hideBatchSeparate: true,
      lyricsFontStep: 2,
    });
  });
});
