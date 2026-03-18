import type { ImportSongsOptions } from "@/types/ipc";

export interface AmbiguousCdgChoiceRequest {
  cdgPath: string;
  audioCandidates: string[];
  stem: string;
}

export interface ExplicitCdgSelection {
  audioPath: string;
  cdgPath: string;
}

function getPathExtension(path: string): string {
  const normalized = path.replace(/\\/g, "/");
  const lastSegment = normalized.split("/").pop() ?? normalized;
  const dotIndex = lastSegment.lastIndexOf(".");
  if (dotIndex < 0) {
    return "";
  }

  return lastSegment.slice(dotIndex + 1).toLowerCase();
}

function getPathStem(path: string): string | null {
  const normalized = path.replace(/\\/g, "/");
  const lastSegment = normalized.split("/").pop() ?? normalized;
  const dotIndex = lastSegment.lastIndexOf(".");
  if (dotIndex <= 0) {
    return null;
  }

  return lastSegment.slice(0, dotIndex).toLowerCase();
}

function getParentDirectory(path: string): string {
  const normalized = path.replace(/\\/g, "/");
  const slashIndex = normalized.lastIndexOf("/");
  if (slashIndex < 0) {
    return "";
  }

  return normalized.slice(0, slashIndex);
}

function buildSiblingCdgPath(audioPath: string): string {
  const normalized = audioPath.replace(/\\/g, "/");
  const dotIndex = normalized.lastIndexOf(".");
  if (dotIndex < 0) {
    return `${normalized}.cdg`;
  }

  return `${normalized.slice(0, dotIndex)}.cdg`;
}

export function buildAmbiguousCdgChoiceRequests(
  paths: string[],
): AmbiguousCdgChoiceRequest[] {
  const audioByStem = new Map<string, string[]>();
  const cdgByStem = new Map<string, string[]>();

  for (const path of paths) {
    const stem = getPathStem(path);
    if (!stem) {
      continue;
    }

    const extension = getPathExtension(path);
    if (extension === "cdg") {
      const bucket = cdgByStem.get(stem) ?? [];
      bucket.push(path);
      cdgByStem.set(stem, bucket);
      continue;
    }

    if (extension === "lrc") {
      continue;
    }

    const bucket = audioByStem.get(stem) ?? [];
    bucket.push(path);
    audioByStem.set(stem, bucket);
  }

  const requests = [...cdgByStem.entries()]
    .flatMap(([stem, cdgPaths]) => {
      const audioCandidates = [...(audioByStem.get(stem) ?? [])].sort();
      if (audioCandidates.length < 2) {
        return [];
      }

      return cdgPaths.map((cdgPath) => ({
        cdgPath,
        audioCandidates,
        stem,
      }));
    })
    .sort((a, b) => a.cdgPath.localeCompare(b.cdgPath));

  for (const [stem, audioPaths] of audioByStem.entries()) {
    if (audioPaths.length < 2) {
      continue;
    }

    const directories = new Set(audioPaths.map(getParentDirectory));
    if (directories.size !== 1) {
      continue;
    }

    const cdgPath = buildSiblingCdgPath(audioPaths[0]);
    if (requests.some((request) => request.cdgPath === cdgPath)) {
      continue;
    }

    requests.push({
      cdgPath,
      audioCandidates: [...audioPaths].sort(),
      stem,
    });
  }

  return requests.sort((a, b) => a.cdgPath.localeCompare(b.cdgPath));
}

export function buildImportSongsOptions(
  selections: ExplicitCdgSelection[],
  skippedAudioPaths: string[] = [],
): ImportSongsOptions | undefined {
  if (selections.length === 0 && skippedAudioPaths.length === 0) {
    return undefined;
  }

  const options: ImportSongsOptions = {
    explicit_cdg_by_audio_path: Object.fromEntries(
      selections.map(({ audioPath, cdgPath }) => [audioPath, cdgPath]),
    ),
  };

  if (skippedAudioPaths.length > 0) {
    options.skip_cdg_for_audio_paths = skippedAudioPaths;
  }

  return options;
}
