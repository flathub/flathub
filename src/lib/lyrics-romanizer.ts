import { isLatinScript } from "lyric-romanizer/detector";
import type { Romanizer } from "lyric-romanizer";

let romanizerPromise: Promise<Romanizer> | null = null;

async function getRomanizer() {
  romanizerPromise ??= import("lyric-romanizer").then(({ createRomanizer }) =>
    createRomanizer({ japaneseDictPath: "/dict/" }),
  );
  return romanizerPromise;
}

export async function romanizeLyricsLines(lines: readonly string[]) {
  if (isLatinScript(lines)) {
    return [...lines];
  }

  const romanizer = await getRomanizer();
  // Romanize line-by-line so mixed-language content (e.g. Japanese +
  // English) doesn't push pure-Latin lines into a non-Latin pipeline.
  // The library's romanizeLines() picks one global script and applies it
  // to every line, which sends English lines through Kuroshiro when the
  // global script is Japanese.
  const result = await Promise.all(
    lines.map(async (line) => {
      if (isLatinScript([line])) return line;
      try {
        const r = await romanizer.romanizeLines([line]);
        return r.lines[0] ?? line;
      } catch {
        return line;
      }
    }),
  );
  return result;
}
