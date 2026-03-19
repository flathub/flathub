import { mkdirSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import {
  fetchReleaseByTag,
  parseArgs,
  releaseDateISO,
  requireArg,
  sha256ForUrl,
} from "./release-metadata.mjs";

const args = parseArgs(process.argv.slice(2));

const owner = args.owner ?? "thedavidweng";
const repo = args.repo ?? "OpenKara";
const version = requireArg(args, "version");
const tag = args.tag ?? `v${version}`;
const outputDir = requireArg(args, "output");
const screenshotBaseUrl = requireArg(args, "screenshot-base-url");
const templateRoot = args["template-root"] ?? "packaging/flatpak";

const release = await fetchReleaseByTag({ owner, repo, tag });
const releaseDate = releaseDateISO(release);
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
  .map(
    (file) =>
      `      - type: file\n        path: packaging/flatpak/generated/${file}`,
  )
  .join("\n");

const manifestTemplate = readFileSync(
  join(templateRoot, "io.github.thedavidweng.OpenKara.yml.in"),
  "utf8",
);
const metainfoTemplate = readFileSync(
  join(templateRoot, "io.github.thedavidweng.OpenKara.metainfo.xml.in"),
  "utf8",
);

const replacements = new Map([
  ["@@APP_VERSION@@", version],
  ["@@SOURCE_URL@@", sourceUrl],
  ["@@SOURCE_SHA256@@", sourceSha256],
  ["@@RELEASE_DATE@@", releaseDate],
  ["@@SCREENSHOT_BASE_URL@@", screenshotBaseUrl],
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
  join(outputRoot, "io.github.thedavidweng.OpenKara.metainfo.xml"),
  replaceTokens(metainfoTemplate),
);
writeFileSync(
  join(outputRoot, "io.github.thedavidweng.OpenKara.desktop"),
  readFileSync(
    join(templateRoot, "io.github.thedavidweng.OpenKara.desktop"),
    "utf8",
  ),
);
writeFileSync(
  join(outputRoot, "flathub.json"),
  readFileSync(join(templateRoot, "flathub.json"), "utf8"),
);
writeFileSync(
  join(outputRoot, "tauri.flatpak.conf.json"),
  readFileSync(join(templateRoot, "tauri.flatpak.conf.json"), "utf8"),
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
