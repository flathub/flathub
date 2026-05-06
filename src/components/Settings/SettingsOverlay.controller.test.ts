// @vitest-environment jsdom

import { beforeEach, describe, expect, test, vi } from "vitest";

vi.mock("@/stores/bootstrap-store", () => ({
  useBootstrapStore: {
    getState: () => ({
      loadStatus: () => Promise.resolve(),
    }),
  },
}));

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
      createLocalLibrary: vi.fn(),
      deleteAllCachedLyrics: vi.fn(),
      deleteAllStems: vi.fn(),
      deleteModel: vi.fn(),
      downloadModel: vi.fn(),
      downgradeAllToTwoStem: vi.fn(),
      estimateDowngradeSavings: vi.fn(),
      estimateStemsSize: vi.fn(),
      getAllSeparationStatuses: vi.fn(),
      getLibraryPath: vi.fn(),
      getLibraryRegistry: vi.fn(),
      getSettings: vi.fn(),
      getModelStatus: vi.fn(),
      openLibrary: vi.fn(),
      registerLocalLibrary: vi.fn(),
      restartApp: vi.fn(),
      switchLibrary: vi.fn(),
      refreshRemoteRepository: vi.fn(),
      renameLibrary: vi.fn(),
      removeLibrary: vi.fn(),
      deleteLibrary: vi.fn(),
      mirrorLocalLibraryToRemote: vi.fn(),
      reauthorizeRemoteLibrary: vi.fn(),
      setExecutionProvider: vi.fn(),
      setHideBatchSeparate: vi.fn(),
      setCoverArtBackdrop: vi.fn(),
      setLanguage: vi.fn(),
      setModelVariant: vi.fn(),
      setStemMode: vi.fn(),
    },
    notifyError: vi.fn(),
    openDirectory: vi.fn(),
    changeLanguage: vi.fn(),
    libraryStore: {
      clearAllSeparationStatuses: vi.fn(),
      clearAllUploadStatuses: vi.fn(),
      clearSelection: vi.fn(),
      loadLibrary: vi.fn(),
      updateSeparationStatus: vi.fn(),
    },
    queueStore: {
      clearQueue: vi.fn(),
    },
    playerStore: {
      loadState: vi.fn(),
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
          coverArtBackdrop: true,
          lyricsFontStep: 0,
          executionProvider: "xnnpack",
          availableExecutionProviders: ["cpu", "xnnpack"],
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

    vi.mocked(harness.dependencies.api.getLibraryRegistry).mockResolvedValue({
      active_library_id: "local:/karaoke",
      libraries: [
        {
          id: "local:/karaoke",
          kind: "local",
          display_name: "karaoke",
          root_path: "/karaoke",
        },
      ],
    });
    vi.mocked(harness.dependencies.api.getSettings).mockResolvedValue({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
      cover_art_backdrop: true,
      lyrics_font_step: 0,
      execution_provider: "xnnpack",
      available_execution_providers: ["cpu", "xnnpack"],
    });
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: true,
        legacy_install_present: false,
        file_size: 123,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      });

    await harness.actions.initialize();

    expect(
      harness.dependencies.settingsStore.getAppSettingsSnapshot,
    ).not.toHaveBeenCalled();
    expect(
      harness.dependencies.settingsStore.hydrateAppSettings,
    ).toHaveBeenCalledWith({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
      cover_art_backdrop: true,
      lyrics_font_step: 0,
      execution_provider: "xnnpack",
      available_execution_providers: ["cpu", "xnnpack"],
    });
    expect(harness.getSnapshot()).toMatchObject({
      state: {
        libraryPath: "/karaoke",
        libraryRegistry: {
          active_library_id: "local:/karaoke",
          libraries: [
            {
              id: "local:/karaoke",
              kind: "local",
              display_name: "karaoke",
              root_path: "/karaoke",
            },
          ],
        },
        libraries: [
          {
            id: "local:/karaoke",
            kind: "local",
            display_name: "karaoke",
            root_path: "/karaoke",
          },
        ],
        activeLibraryId: "local:/karaoke",
        stemMode: "four_stem",
        modelVariant: "htdemucs_ft",
        language: "zh-CN",
        hideBatchSeparate: true,
        modelStatuses: {},
      },
      meta: {
        isInitializing: false,
      },
    });

    await Promise.resolve();
    await Promise.resolve();

    expect(harness.getSnapshot()).toMatchObject({
      state: {
        modelStatuses: {
          htdemucs: {
            downloaded: true,
            legacy_install_present: false,
            file_size: 123,
          },
          htdemucs_ft: {
            downloaded: false,
            legacy_install_present: false,
            file_size: null,
          },
        },
      },
    });
  });

  test("creating a local library refreshes the registry snapshot", async () => {
    const harness = createControllerHarness();

    vi.mocked(harness.dependencies.openDirectory).mockResolvedValue("/music");
    vi.mocked(harness.dependencies.api.createLocalLibrary).mockResolvedValue(
      undefined,
    );
    vi.mocked(harness.dependencies.api.getLibraryRegistry).mockResolvedValue({
      active_library_id: "local:/music/OpenKara",
      libraries: [
        {
          id: "local:/music/OpenKara",
          kind: "local",
          display_name: "OpenKara",
          root_path: "/music/OpenKara",
        },
      ],
    });

    await harness.actions.createLibrary("Create library");

    expect(harness.dependencies.api.createLocalLibrary).toHaveBeenCalledWith(
      "/music/OpenKara",
    );
    expect(harness.getSnapshot().state.activeLibraryId).toBe(
      "local:/music/OpenKara",
    );
    expect(harness.getSnapshot().state.libraryPath).toBe("/music/OpenKara");
    expect(harness.getSnapshot().state.libraries).toHaveLength(1);
  });

  test("switching libraries clears transient state and syncs active remotes", async () => {
    const harness = createControllerHarness();

    vi.mocked(harness.dependencies.api.switchLibrary).mockResolvedValue({
      active_library_id: "remote:library-1",
      libraries: [
        {
          id: "remote:library-1",
          kind: "remote",
          display_name: "Drive",
          provider: "google_drive",
          account_id: "account-1",
          remote_root_locator: "root-1",
          remote_path_display: "Google Drive Library",
          connection_config: null,
          cached_db_path: "/tmp/drive/library.sqlite3",
          remote_revision: null,
        },
      ],
    });
    vi.mocked(harness.dependencies.api.getLibraryRegistry).mockResolvedValue({
      active_library_id: "remote:library-1",
      libraries: [
        {
          id: "remote:library-1",
          kind: "remote",
          display_name: "Drive",
          provider: "google_drive",
          account_id: "account-1",
          remote_root_locator: "root-1",
          remote_path_display: "Google Drive Library",
          connection_config: null,
          cached_db_path: "/tmp/drive/library.sqlite3",
          remote_revision: null,
        },
      ],
    });
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: true,
        legacy_install_present: false,
        file_size: 1,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      });

    await harness.actions.switchLibrary("remote:library-1");

    expect(harness.dependencies.api.switchLibrary).toHaveBeenCalledWith(
      "remote:library-1",
    );
    expect(
      harness.dependencies.api.refreshRemoteRepository,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.clearAllSeparationStatuses,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.clearAllUploadStatuses,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.clearSelection,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.loadLibrary,
    ).toHaveBeenCalledOnce();
    expect(harness.dependencies.queueStore.clearQueue).toHaveBeenCalledOnce();
    expect(harness.dependencies.lyricsStore.clear).toHaveBeenCalledOnce();
    expect(harness.dependencies.playerStore.loadState).toHaveBeenCalledOnce();
    expect(harness.getSnapshot().state.activeLibraryId).toBe(
      "remote:library-1",
    );
  });

  test("refreshing the active remote repository updates the working copy without switching", async () => {
    const harness = createControllerHarness();

    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        activeLibraryId: "remote:library-1",
        libraries: [
          {
            id: "remote:library-1",
            kind: "remote",
            display_name: "Drive",
            provider: "google_drive",
            account_id: "account-1",
            remote_root_locator: "root-1",
            remote_path_display: "Google Drive Library",
            connection_config: null,
            cached_db_path: "/tmp/drive/library.sqlite3",
            remote_revision: "rev-1",
          },
        ],
      },
      meta: harness.getSnapshot().meta,
    });
    vi.mocked(
      harness.dependencies.api.refreshRemoteRepository,
    ).mockResolvedValue(undefined);
    vi.mocked(harness.dependencies.api.getLibraryRegistry).mockResolvedValue({
      active_library_id: "remote:library-1",
      libraries: [
        {
          id: "remote:library-1",
          kind: "remote",
          display_name: "Drive",
          provider: "google_drive",
          account_id: "account-1",
          remote_root_locator: "root-1",
          remote_path_display: "Google Drive Library",
          connection_config: null,
          cached_db_path: "/tmp/drive/library.sqlite3",
          remote_revision: "rev-2",
        },
      ],
    });
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: true,
        legacy_install_present: false,
        file_size: 1,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      });

    await harness.actions.refreshRemoteRepository("remote:library-1");

    expect(harness.dependencies.api.switchLibrary).not.toHaveBeenCalled();
    expect(
      harness.dependencies.api.refreshRemoteRepository,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.clearAllSeparationStatuses,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.clearAllUploadStatuses,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.clearSelection,
    ).toHaveBeenCalledOnce();
    expect(
      harness.dependencies.libraryStore.loadLibrary,
    ).toHaveBeenCalledOnce();
    expect(harness.dependencies.queueStore.clearQueue).toHaveBeenCalledOnce();
    expect(harness.dependencies.lyricsStore.clear).toHaveBeenCalledOnce();
    expect(harness.dependencies.playerStore.loadState).toHaveBeenCalledOnce();
    const refreshedLibrary =
      harness.getSnapshot().state.libraryRegistry?.libraries[0];
    expect(refreshedLibrary?.kind).toBe("remote");
    expect(
      refreshedLibrary?.kind === "remote"
        ? refreshedLibrary.remote_revision
        : null,
    ).toBe("rev-2");
  });

  test("delete remote repository confirmation names provider-hosted content and requires display-name confirmation", async () => {
    const harness = createControllerHarness();
    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        activeLibraryId: "remote:library-1",
        libraries: [
          {
            id: "remote:library-1",
            kind: "remote",
            display_name: "Drive",
            provider: "google_drive",
            account_id: "account-1",
            remote_root_locator: "root-1",
            remote_path_display: "Google Drive / OpenKara",
            connection_config: null,
            cached_db_path: "/tmp/drive/library.sqlite3",
            remote_revision: "rev-1",
          },
        ],
      },
      meta: harness.getSnapshot().meta,
    });
    const confirm = vi.spyOn(window, "confirm").mockReturnValue(true);
    const prompt = vi.spyOn(window, "prompt").mockReturnValue("Drive");
    vi.mocked(harness.dependencies.api.deleteLibrary).mockResolvedValue({
      active_library_id: null,
      libraries: [],
    });

    await harness.actions.deleteLibrary("remote:library-1");

    expect(confirm).toHaveBeenCalledWith(
      expect.stringContaining(
        "delete the remote repository contents from Google Drive",
      ),
    );
    expect(confirm).toHaveBeenCalledWith(
      expect.stringContaining("Google Drive / OpenKara"),
    );
    expect(prompt).toHaveBeenCalledWith(
      expect.stringContaining('Type "Drive"'),
      "",
    );
    expect(harness.dependencies.api.deleteLibrary).toHaveBeenCalledWith(
      "remote:library-1",
    );
  });

  test("delete remote repository is cancelled when display-name confirmation does not match", async () => {
    const harness = createControllerHarness();
    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        activeLibraryId: "remote:library-1",
        libraries: [
          {
            id: "remote:library-1",
            kind: "remote",
            display_name: "Drive",
            provider: "dropbox",
            account_id: "account-1",
            remote_root_locator: "/OpenKara",
            remote_path_display: "/OpenKara",
            connection_config: null,
            cached_db_path: "/tmp/drive/library.sqlite3",
            remote_revision: "rev-1",
          },
        ],
      },
      meta: harness.getSnapshot().meta,
    });
    vi.spyOn(window, "confirm").mockReturnValue(true);
    vi.spyOn(window, "prompt").mockReturnValue("Wrong");

    await harness.actions.deleteLibrary("remote:library-1");

    expect(harness.dependencies.api.deleteLibrary).not.toHaveBeenCalled();
  });

  test("selecting an undownloaded model downloads it before applying the variant", async () => {
    const harness = createControllerHarness();

    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        modelVariant: "htdemucs_ft",
        modelStatuses: {
          htdemucs: {
            downloaded: false,
            legacy_install_present: false,
            file_size: null,
          },
          htdemucs_ft: {
            downloaded: true,
            legacy_install_present: false,
            file_size: 10,
          },
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
        legacy_install_present: false,
        file_size: 2048,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: true,
        legacy_install_present: false,
        file_size: 4096,
      });
    vi.mocked(harness.dependencies.api.setModelVariant).mockResolvedValue({
      stem_mode: "two_stem",
      model_variant: "htdemucs",
      language: "en",
      hide_batch_separate: false,
      cover_art_backdrop: true,
      lyrics_font_step: 0,
      execution_provider: "xnnpack",
      available_execution_providers: ["cpu", "xnnpack"],
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

  test("deleting the active model is allowed when install is verified", async () => {
    const harness = createControllerHarness();

    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        modelVariant: "htdemucs",
        modelStatuses: {
          htdemucs: {
            downloaded: true,
            legacy_install_present: false,
            file_size: 100,
          },
          htdemucs_ft: {
            downloaded: false,
            legacy_install_present: false,
            file_size: null,
          },
        },
      },
      meta: harness.getSnapshot().meta,
    });

    vi.mocked(harness.dependencies.api.deleteModel).mockResolvedValue(
      undefined,
    );
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      });

    await harness.actions.deleteModel("htdemucs");

    expect(harness.dependencies.api.deleteModel).toHaveBeenCalledWith(
      "htdemucs",
    );
  });

  test("deleting the active model is allowed when legacy install is present", async () => {
    const harness = createControllerHarness();

    harness.setSnapshot({
      state: {
        ...harness.getSnapshot().state,
        modelVariant: "htdemucs",
        modelStatuses: {
          htdemucs: {
            downloaded: false,
            legacy_install_present: true,
            file_size: 999,
          },
          htdemucs_ft: {
            downloaded: false,
            legacy_install_present: false,
            file_size: null,
          },
        },
      },
      meta: harness.getSnapshot().meta,
    });

    vi.mocked(harness.dependencies.api.deleteModel).mockResolvedValue(
      undefined,
    );
    vi.mocked(harness.dependencies.api.getModelStatus)
      .mockResolvedValueOnce({
        variant: "htdemucs",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      })
      .mockResolvedValueOnce({
        variant: "htdemucs_ft",
        downloaded: false,
        legacy_install_present: false,
        file_size: null,
      });

    await harness.actions.deleteModel("htdemucs");

    expect(harness.dependencies.api.deleteModel).toHaveBeenCalledWith(
      "htdemucs",
    );
  });

  test("hide batch separate updates local state and the settings store", async () => {
    const harness = createControllerHarness();
    vi.mocked(harness.dependencies.api.setHideBatchSeparate).mockResolvedValue({
      stem_mode: "four_stem",
      model_variant: "htdemucs_ft",
      language: "zh-CN",
      hide_batch_separate: true,
      cover_art_backdrop: true,
      lyrics_font_step: 0,
      execution_provider: "xnnpack",
      available_execution_providers: ["cpu", "xnnpack"],
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

  test("restart app delegates to the backend restart command", async () => {
    const harness = createControllerHarness();

    await harness.actions.restartApp();

    expect(harness.dependencies.api.restartApp).toHaveBeenCalledOnce();
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
