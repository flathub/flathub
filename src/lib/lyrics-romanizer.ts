import { isLatinScript } from "lyric-romanizer/detector";
import type { Romanizer } from "lyric-romanizer";

let romanizerPromise: Promise<Romanizer> | null = null;

async function getRomanizer() {
  romanizerPromise ??= import("lyric-romanizer").then(({ createRomanizer }) =>
    createRomanizer(),
  );
  return romanizerPromise;
}

export async function romanizeLyricsLines(lines: readonly string[]) {
  if (isLatinScript(lines)) {
    return [...lines];
  }

  const romanizer = await getRomanizer();
  const result = await romanizer.romanizeLines(lines);
  return result.lines;
}
