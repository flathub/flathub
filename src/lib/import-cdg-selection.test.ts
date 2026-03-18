import { describe, expect, test } from "vitest";
import {
  buildAmbiguousCdgChoiceRequests,
  buildImportSongsOptions,
} from "./import-cdg-selection";

describe("import cdg selection helpers", () => {
  test("finds ambiguous CDG matches when one CDG matches multiple selected audio files", () => {
    const requests = buildAmbiguousCdgChoiceRequests([
      "/tmp/track.mp3",
      "/tmp/track.flac",
      "/tmp/track.cdg",
    ]);

    expect(requests).toEqual([
      {
        cdgPath: "/tmp/track.cdg",
        audioCandidates: ["/tmp/track.flac", "/tmp/track.mp3"],
        stem: "track",
      },
    ]);
  });

  test("finds ambiguous sibling CDG matches when multiple selected audio files share one sidecar", () => {
    const requests = buildAmbiguousCdgChoiceRequests([
      "/tmp/track.mp3",
      "/tmp/track.flac",
    ]);

    expect(requests).toEqual([
      {
        cdgPath: "/tmp/track.cdg",
        audioCandidates: ["/tmp/track.flac", "/tmp/track.mp3"],
        stem: "track",
      },
    ]);
  });

  test("builds explicit audio-to-cdg selections for the import command", () => {
    expect(
      buildImportSongsOptions(
        [
          {
            audioPath: "/tmp/track.flac",
            cdgPath: "/tmp/track.cdg",
          },
        ],
        ["/tmp/track.mp3"],
      ),
    ).toEqual({
      explicit_cdg_by_audio_path: {
        "/tmp/track.flac": "/tmp/track.cdg",
      },
      skip_cdg_for_audio_paths: ["/tmp/track.mp3"],
    });
  });
});
