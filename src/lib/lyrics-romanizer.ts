import { isLatinScript } from "lyric-romanizer/detector";
import type { Romanizer, RomanizeOptions } from "lyric-romanizer";
import type { SongLanguage } from "@/components/Library/song-list-item-menu";

let romanizerPromise: Promise<Romanizer> | null = null;

async function getRomanizer() {
  romanizerPromise ??= import("lyric-romanizer").then(({ createRomanizer }) =>
    createRomanizer({ japaneseDictPath: "/dict/" }),
  );
  return romanizerPromise;
}

function buildOptions(
  language: SongLanguage | null,
): RomanizeOptions | undefined {
  if (language === "cantonese") {
    return { script: "chinese", dialect: "cantonese" };
  }
  return undefined;
}

export async function romanizeLyricsLines(
  lines: readonly string[],
  language?: SongLanguage | null,
) {
  if (isLatinScript(lines)) {
    return [...lines];
  }

  const romanizer = await getRomanizer();
  const options = buildOptions(language ?? null);
  const result = await Promise.all(
    lines.map(async (line) => {
      if (isLatinScript([line])) return line;
      try {
        const r = await romanizer.romanizeLines([line], options);
        return r.lines[0] ?? line;
      } catch {
        return line;
      }
    }),
  );
  return result;
}
