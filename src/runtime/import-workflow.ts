import type { TFunction } from "i18next";
import type { ImportFailure, Song } from "@/types/ipc";
import {
  buildAmbiguousCdgChoiceRequests,
  buildImportSongsOptions,
  type AmbiguousCdgChoiceRequest,
  type ExplicitCdgSelection,
} from "@/lib/import-cdg-selection";

export interface ImportWorkflowApi {
  importSongs: typeof import("@/lib/tauri").importSongs;
  importLyricsFiles: typeof import("@/lib/tauri").importLyricsFiles;
  getLibrary: typeof import("@/lib/tauri").getLibrary;
}

export interface RunImportWorkflowOptions {
  paths: string[];
  api: ImportWorkflowApi;
  promptForCdgChoice: (
    request: AmbiguousCdgChoiceRequest,
  ) => Promise<string | null>;
  notifyError: (error: unknown) => void;
  setImportErrors: (errors: ImportFailure[]) => void;
  setSongs: (songs: Song[]) => void;
  publishLibraryInvalidation: () => void;
  t?: TFunction;
}

export async function runImportWorkflow({
  paths,
  api,
  promptForCdgChoice,
  notifyError,
  setImportErrors,
  setSongs,
  publishLibraryInvalidation,
}: RunImportWorkflowOptions) {
  const audioPaths = paths.filter((p) => !p.toLowerCase().endsWith(".lrc"));
  const lrcPaths = paths.filter((p) => p.toLowerCase().endsWith(".lrc"));
  const explicitSelections: ExplicitCdgSelection[] = [];
  const excludedAmbiguousAudioPaths = new Set<string>();

  for (const request of buildAmbiguousCdgChoiceRequests(audioPaths)) {
    const selectedAudioPath = await promptForCdgChoice(request);
    if (selectedAudioPath) {
      for (const candidate of request.audioCandidates) {
        if (candidate !== selectedAudioPath) {
          excludedAmbiguousAudioPaths.add(candidate);
        }
      }
      explicitSelections.push({
        audioPath: selectedAudioPath,
        cdgPath: request.cdgPath,
      });
    }
  }

  const audioPathsToImport = audioPaths.filter(
    (path) => !excludedAmbiguousAudioPaths.has(path),
  );

  if (audioPathsToImport.length > 0) {
    const result = await api.importSongs(
      audioPathsToImport,
      buildImportSongsOptions(explicitSelections),
    );
    if (result.failed.length > 0) {
      setImportErrors(result.failed);
      for (const failure of result.failed) {
        notifyError(failure.error);
      }
    }
  }

  if (lrcPaths.length > 0) {
    const lrcResult = await api.importLyricsFiles(lrcPaths);
    if (lrcResult.unmatched.length > 0) {
      for (const path of lrcResult.unmatched) {
        notifyError(`Lyrics file could not be matched to a song: ${path}`);
      }
    }
  }

  const songs = await api.getLibrary();
  setSongs(songs);
  publishLibraryInvalidation();
}
