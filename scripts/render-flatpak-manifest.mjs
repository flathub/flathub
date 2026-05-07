import { mkdirSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { parseArgs, requireArg, sha256ForUrl } from "./release-metadata.mjs";

const args = parseArgs(process.argv.slice(2));

const owner = args.owner ?? "thedavidweng";
const repo = args.repo ?? "OpenKara";
const version = requireArg(args, "version");
const tag = args.tag ?? `v${version}`;
const outputDir = requireArg(args, "output");
const templateRoot = args["template-root"] ?? "packaging/flatpak";

const sourceUrl =
  args["source-url"] ??
  `https://github.com/${owner}/${repo}/archive/refs/tags/${tag}.tar.gz`;
const sourceSha256 = await sha256ForUrl(sourceUrl);

const outputRoot = join(outputDir, "io.github.thedavidweng.OpenKara");
mkdirSync(outputRoot, { recursive: true });

const cargoSourcesPath = join(templateRoot, "generated", "cargo-sources.json");
const nodeSourceFiles = readdirSync(join(templateRoot, "generated"))
  .filter((file) => file.startsWith("node-sources") && file.endsWith(".json"))
  .sort();

const nodeSourcesYaml = nodeSourceFiles
  .map((file) => `      - ${file}`)
  .join("\n");

const manifestTemplate = readFileSync(
  join(templateRoot, "io.github.thedavidweng.OpenKara.yml.in"),
  "utf8",
);

const replacements = new Map([
  ["@@SOURCE_URL@@", sourceUrl],
  ["@@SOURCE_SHA256@@", sourceSha256],
  ["@@NODE_SOURCES@@", nodeSourcesYaml],
]);

const replaceTokens = (content) => {
  let next = content;
  for (const [token, value] of replacements.entries()) {
    next = next.replaceAll(token, value);
  }
  return next;
};

writeFileSync(
  join(outputRoot, "io.github.thedavidweng.OpenKara.yml"),
  replaceTokens(manifestTemplate),
);
writeFileSync(
  join(outputRoot, "cargo-sources.json"),
  readFileSync(cargoSourcesPath, "utf8"),
);

for (const file of nodeSourceFiles) {
  writeFileSync(
    join(outputRoot, file),
    readFileSync(join(templateRoot, "generated", file), "utf8"),
  );
}

process.stdout.write(`${outputRoot}\n`);
