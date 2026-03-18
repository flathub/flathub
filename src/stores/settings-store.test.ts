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
    });
  });

  test("hydrateAppSettings stores the full app settings snapshot", () => {
    useSettingsStore.getState().hydrateAppSettings({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
    });

    expect(useSettingsStore.getState()).toMatchObject({
      hydrated: true,
      stemMode: "four_stem",
      modelVariant: "htdemucs_ft",
      language: "zh-CN",
      hideBatchSeparate: true,
    });
  });

  test("patchAppSettings updates the shared snapshot without resetting other fields", () => {
    useSettingsStore.getState().hydrateAppSettings({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: false,
    });

    useSettingsStore.getState().patchAppSettings({
      language: "en",
      hideBatchSeparate: true,
    });

    expect(useSettingsStore.getState().getAppSettingsSnapshot()).toEqual({
      hydrated: true,
      stemMode: "four_stem",
      modelVariant: "htdemucs_ft",
      language: "en",
      hideBatchSeparate: true,
    });
  });
});
