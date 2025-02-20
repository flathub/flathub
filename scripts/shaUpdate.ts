import { $ } from "jsr:@david/dax@0.42.0";

const targetFile = Deno.args[0];
if (import.meta.main) {
  const update = async (fileContent: string) => {
    const result =
      await $`flatpak-builder build-dir ${targetFile} --force-clean --user --install`
        .stderr("inheritPiped")
        .noThrow()
        .then((r) => r.stderr);
    const expected = result.match(/expected "(.*?)"/)?.at(1);
    const was = result.match(/was "(.*?)"/)?.at(1);
    if (!expected || !was) return undefined;

    console.log("expected: ", expected);
    console.log("was: ", was);
    return fileContent.replace(expected, was);
  };

  while (true) {
    const newContent = await update(Deno.readTextFileSync(targetFile));
    if (!newContent) break;
    Deno.writeTextFileSync(
      targetFile,
      newContent,
    );
  }
}
