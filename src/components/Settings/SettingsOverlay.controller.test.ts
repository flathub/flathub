import { beforeEach, describe, expect, test, vi } from "vitest";
import {
  createInitialSettingsOverlaySnapshot,
  createSettingsOverlayActions,
  type SettingsOverlayControllerDependencies,
  type SettingsOverlaySnapshot,
} from "./SettingsOverlay.state";
import type { AppSettingsSnapshot } from "@/stores/settings-store";
import type { SeparationStatusSnapshot } from "@/types/ipc";

function createControllerHarness() {
  let snapshot: SettingsOverlaySnapshot =
    createInitialSettingsOverlaySnapshot();

  const dependencies: SettingsOverlayControllerDependencies = {
    api: {
      createLibrary: vi.fn(),
      deleteAllCachedLyrics: vi.fn(),
      deleteAllStems: vi.fn(),
      deleteModel: vi.fn(),
      downloadModel: vi.fn(),
      downgradeAllToTwoStem: vi.fn(),
      estimateDowngradeSavings: vi.fn(),
      estimateStemsSize: vi.fn(),
      getAllSeparationStatuses: vi.fn(),
      getLibraryPath: vi.fn(),
      getModelStatus: vi.fn(),
      openLibrary: vi.fn(),
      setHideBatchSeparate: vi.fn(),
      setLanguage: vi.fn(),
      setModelVariant: vi.fn(),
      setStemMode: vi.fn(),
    },
    notifyError: vi.fn(),
    openDirectory: vi.fn(),
    changeLanguage: vi.fn(),
    libraryStore: {
      clearAllSeparationStatuses: vi.fn(),
      updateSeparationStatus: vi.fn(),
    },
    lyricsStore: {
      clear: vi.fn(),
    },
    settingsStore: {
      getAppSettingsSnapshot: vi.fn(
        (): AppSettingsSnapshot => ({
          hydrated: true,
          stemMode: "four_stem",
          modelVariant: "htdemucs_ft",
          language: "zh-CN",
          hideBatchSeparate: true,
        }),
      ),
      hydrateAppSettings: vi.fn(),
      patchAppSettings: vi.fn(),
    },
  };

  const actions = createSettingsOverlayActions(dependencies, {
    getSnapshot: () => snapshot,
    setSnapshot: (updater) => {
      snapshot = updater(snapshot);
    },
  });

  return {
    actions,
    dependencies,
    getSnapshot: () => snapshot,
    setSnapshot: (next: SettingsOverlaySnapshot) => {
      snapshot = next;
    },
  };
}

describe("SettingsOverlay controller", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  test("initial load populates library path, settings, and model statuses", async () => {
    const harness = createControllerHarness();

    vi.mocked(harness.dependencies.api.getLibraryPath).mockResolvedValue(
      "/karaoke",
    );
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: true,
        file_size: 123,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: false,
        file_size: null,
      });

    await harness.actions.initialize();

    expect(
      harness.dependencies.settingsStore.getAppSettingsSnapshot,
    ).toHaveBeenCalled();
    expect(
      harness.dependencies.settingsStore.hydrateAppSettings,
    ).not.toHaveBeenCalled();
    expect(harness.getSnapshot()).toMatchObject({
      state: {
        libraryPath: "/karaoke",
        stemMode: "four_stem",
        modelVariant: "htdemucs_ft",
        language: "zh-CN",
        hideBatchSeparate: true,
        modelStatuses: {
          htdemucs: { downloaded: true, file_size: 123 },
          htdemucs_ft: { downloaded: false, file_size: null },
        },
      },
      meta: {
        isInitializing: false,
      },
    });
  });

  test("selecting an undownloaded model downloads it before applying the variant", async () => {
    const harness = createControllerHarness();

    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        modelVariant: "htdemucs_ft",
        modelStatuses: {
          htdemucs: { downloaded: false, file_size: null },
          htdemucs_ft: { downloaded: true, file_size: 10 },
        },
      },
      meta: harness.getSnapshot().meta,
    });

    vi.mocked(harness.dependencies.api.downloadModel).mockResolvedValue({
      state: "ready",
      model_path: "/tmp/model",
      downloaded_bytes: null,
      total_bytes: null,
      error: null,
    });
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: true,
        file_size: 2048,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: true,
        file_size: 4096,
      });
    vi.mocked(harness.dependencies.api.setModelVariant).mockResolvedValue({
      stem_mode: "two_stem",
      model_variant: "htdemucs",
      language: "en",
      hide_batch_separate: false,
    });

    await harness.actions.selectModelVariant("htdemucs");

    expect(harness.dependencies.api.downloadModel).toHaveBeenCalledWith(
      "htdemucs",
    );
    expect(harness.dependencies.api.setModelVariant).toHaveBeenCalledWith(
      "htdemucs",
    );
    expect(harness.getSnapshot().state.modelVariant).toBe("htdemucs");
    expect(harness.getSnapshot().state.downloadingModel).toBeNull();
  });

  test("selecting the fine-tuned model opens the warning dialog before applying", async () => {
    const harness = createControllerHarness();

    await harness.actions.selectModelVariant("htdemucs_ft");

    expect(harness.getSnapshot().meta.dangerDialog).toBe("ft_warning");
    expect(harness.dependencies.api.downloadModel).not.toHaveBeenCalled();
    expect(harness.dependencies.api.setModelVariant).not.toHaveBeenCalled();
  });

  test("deleting the active model is ignored", async () => {
    const harness = createControllerHarness();

    await harness.actions.deleteModel("htdemucs");

    expect(harness.dependencies.api.deleteModel).not.toHaveBeenCalled();
  });

  test("hide batch separate updates local state and the settings store", async () => {
    const harness = createControllerHarness();
    vi.mocked(harness.dependencies.api.setHideBatchSeparate).mockResolvedValue({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
    });

    await harness.actions.toggleHideBatchSeparate(true);

    expect(harness.getSnapshot().state.hideBatchSeparate).toBe(true);
    expect(
      harness.dependencies.settingsStore.patchAppSettings,
    ).toHaveBeenCalledWith({ hideBatchSeparate: true });
    expect(
      harness.dependencies.settingsStore.hydrateAppSettings,
    ).toHaveBeenCalled();
    expect(harness.dependencies.api.setHideBatchSeparate).toHaveBeenCalledWith(
      true,
    );
  });

  test("delete stems clears in-memory separation statuses after success", async () => {
    const harness = createControllerHarness();

    vi.mocked(harness.dependencies.api.estimateStemsSize).mockResolvedValue(
      512,
    );

    await harness.actions.openDeleteStemsDialog();
    await harness.actions.confirmDeleteStems();

    expect(harness.dependencies.api.deleteAllStems).toHaveBeenCalled();
    expect(
      harness.dependencies.libraryStore.clearAllSeparationStatuses,
    ).toHaveBeenCalled();
    expect(harness.getSnapshot().meta.dangerDialog).toBeNull();
    expect(harness.getSnapshot().meta.deletingStemsInProgress).toBe(false);
  });

  test("downgrade refreshes statuses and repopulates the library store", async () => {
    const harness = createControllerHarness();
    const statuses: SeparationStatusSnapshot[] = [
      {
        song_id: "song-1",
        state: "completed",
        percent: 100,
        cache_hit: false,
        vocals_path: "vocals.ogg",
        accomp_path: "accomp.ogg",
        drums_path: null,
        bass_path: null,
        other_path: null,
        model_variant: "htdemucs",
        error: null,
      },
    ];

    vi.mocked(
      harness.dependencies.api.estimateDowngradeSavings,
    ).mockResolvedValue(4096);
    vi.mocked(
      harness.dependencies.api.getAllSeparationStatuses,
    ).mockResolvedValue(statuses);

    await harness.actions.openDowngradeDialog();
    await harness.actions.confirmDowngrade();

    expect(harness.dependencies.api.downgradeAllToTwoStem).toHaveBeenCalled();
    expect(
      harness.dependencies.libraryStore.clearAllSeparationStatuses,
    ).toHaveBeenCalled();
    expect(
      harness.dependencies.libraryStore.updateSeparationStatus,
    ).toHaveBeenCalledWith(statuses[0]);
    expect(harness.getSnapshot().meta.dangerDialog).toBeNull();
  });

  test("delete cached lyrics clears the lyrics store after success", async () => {
    const harness = createControllerHarness();

    harness.actions.openDeleteLyricsDialog();
    await harness.actions.confirmDeleteLyrics();

    expect(harness.dependencies.api.deleteAllCachedLyrics).toHaveBeenCalled();
    expect(harness.dependencies.lyricsStore.clear).toHaveBeenCalled();
    expect(harness.getSnapshot().meta.dangerDialog).toBeNull();
  });

  test("opening one dialog replaces the previous dialog and closeDialog clears it", async () => {
    const harness = createControllerHarness();

    vi.mocked(harness.dependencies.api.estimateStemsSize).mockResolvedValue(
      100,
    );
    vi.mocked(
      harness.dependencies.api.estimateDowngradeSavings,
    ).mockResolvedValue(200);

    await harness.actions.openDeleteStemsDialog();
    expect(harness.getSnapshot().meta.dangerDialog).toBe("delete_stems");

    await harness.actions.openDowngradeDialog();
    expect(harness.getSnapshot().meta.dangerDialog).toBe("downgrade_stems");

    harness.actions.closeDialog();
    expect(harness.getSnapshot().meta.dangerDialog).toBeNull();
  });

  test("per-action loading flags reset after a dialog confirm flow", async () => {
    const harness = createControllerHarness();

    vi.mocked(harness.dependencies.api.estimateStemsSize).mockResolvedValue(
      256,
    );
    await harness.actions.openDeleteStemsDialog();
    await harness.actions.confirmDeleteStems();

    expect(harness.getSnapshot().meta.deletingStemsInProgress).toBe(false);
    expect(harness.getSnapshot().meta.downgradingInProgress).toBe(false);
    expect(harness.getSnapshot().meta.deletingLyricsInProgress).toBe(false);
  });
});
