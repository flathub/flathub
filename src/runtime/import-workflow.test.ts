import { describe, expect, test, vi } from "vitest";
import { runImportWorkflow, type ImportWorkflowApi } from "./import-workflow";

function workflowApi(): ImportWorkflowApi {
  return {
    importSongs: vi.fn().mockResolvedValue({ imported: [], failed: [] }),
    importLyricsFiles: vi
      .fn()
      .mockResolvedValue({ matched: [], unmatched: [] }),
    getLibrary: vi.fn().mockResolvedValue([]),
  };
}

describe("import workflow", () => {
  test("prompts for ambiguous CDG selections before importing songs", async () => {
    const api = workflowApi();
    const promptForCdgChoice = vi.fn().mockResolvedValue("/tmp/track.flac");
    const setSongs = vi.fn();
    const publishLibraryInvalidation = vi.fn();

    await runImportWorkflow({
      paths: ["/tmp/track.mp3", "/tmp/track.flac", "/tmp/track.cdg"],
      api,
      promptForCdgChoice,
      notifyError: vi.fn(),
      setImportErrors: vi.fn(),
      setSongs,
      publishLibraryInvalidation,
    });

    expect(promptForCdgChoice).toHaveBeenCalledWith({
      cdgPath: "/tmp/track.cdg",
      audioCandidates: ["/tmp/track.flac", "/tmp/track.mp3"],
      stem: "track",
    });
    expect(api.importSongs).toHaveBeenCalledWith(
      ["/tmp/track.flac", "/tmp/track.cdg"],
      {
        explicit_cdg_by_audio_path: {
          "/tmp/track.flac": "/tmp/track.cdg",
        },
      },
    );
    expect(setSongs).toHaveBeenCalledWith([]);
    expect(publishLibraryInvalidation).toHaveBeenCalledTimes(1);
  });

  test("imports lyrics after audio so matching can use imported songs", async () => {
    const api = workflowApi();
    const calls: string[] = [];
    vi.mocked(api.importSongs).mockImplementation(async () => {
      calls.push("audio");
      return { imported: [], failed: [] };
    });
    vi.mocked(api.importLyricsFiles).mockImplementation(async () => {
      calls.push("lyrics");
      return { matched: [], unmatched: [] };
    });

    await runImportWorkflow({
      paths: ["/tmp/song.mp3", "/tmp/song.lrc"],
      api,
      promptForCdgChoice: vi.fn(),
      notifyError: vi.fn(),
      setImportErrors: vi.fn(),
      setSongs: vi.fn(),
      publishLibraryInvalidation: vi.fn(),
    });

    expect(calls).toEqual(["audio", "lyrics"]);
  });

  test("reports import failures and unmatched lyrics", async () => {
    const api = workflowApi();
    const notifyError = vi.fn();
    const setImportErrors = vi.fn();
    const failure = {
      path: "/tmp/bad.mp3",
      error: {
        code: "media_read_failed" as const,
        message: "bad file",
        retryable: false,
        fallback: "reimport_song" as const,
      },
    };
    vi.mocked(api.importSongs).mockResolvedValue({
      imported: [],
      failed: [failure],
    });
    vi.mocked(api.importLyricsFiles).mockResolvedValue({
      matched: [],
      unmatched: ["/tmp/missing.lrc"],
    });

    await runImportWorkflow({
      paths: ["/tmp/bad.mp3", "/tmp/missing.lrc"],
      api,
      promptForCdgChoice: vi.fn(),
      notifyError,
      setImportErrors,
      setSongs: vi.fn(),
      publishLibraryInvalidation: vi.fn(),
    });

    expect(setImportErrors).toHaveBeenCalledWith([failure]);
    expect(notifyError).toHaveBeenCalledWith(failure.error);
    expect(notifyError).toHaveBeenCalledWith(
      "Lyrics file could not be matched to a song: /tmp/missing.lrc",
    );
  });
});
