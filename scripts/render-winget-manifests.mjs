import { mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import {
  fetchReleaseByTag,
  parseArgs,
  requireArg,
  requireReleaseAsset,
  sha256ForUrl,
} from "./release-metadata.mjs";

const args = parseArgs(process.argv.slice(2));

const owner = args.owner ?? "thedavidweng";
const repo = args.repo ?? "OpenKara";
const version = requireArg(args, "version");
const tag = args.tag ?? `v${version}`;
const outputDir = requireArg(args, "output");
const packageIdentifier = args["package-identifier"] ?? "thedavidweng.OpenKara";
const publisher = args.publisher ?? "thedavidweng";
const publisherUrl = args["publisher-url"] ?? "https://github.com/thedavidweng";
const packageUrl =
  args["package-url"] ?? "https://github.com/thedavidweng/OpenKara";
const supportUrl =
  args["support-url"] ?? "https://github.com/thedavidweng/OpenKara/issues";
const installerName =
  args["installer-name"] ?? `OpenKara_${version}_x64-setup.exe`;

const release = await fetchReleaseByTag({ owner, repo, tag });
const installer = requireReleaseAsset(release, installerName);
const installerSha256 =
  installer.digest?.replace(/^sha256:/, "") ??
  (await sha256ForUrl(installer.browser_download_url));

const manifestDir = join(outputDir, "t", "thedavidweng", "OpenKara", version);
const templateRoot = "packaging/winget/templates";

mkdirSync(manifestDir, { recursive: true });
const replacements = new Map([
  ["@@PACKAGE_IDENTIFIER@@", packageIdentifier],
  ["@@PACKAGE_VERSION@@", version],
  ["@@PUBLISHER@@", publisher],
  ["@@PUBLISHER_URL@@", publisherUrl],
  ["@@SUPPORT_URL@@", supportUrl],
  ["@@PACKAGE_URL@@", packageUrl],
  ["@@INSTALLER_URL@@", installer.browser_download_url],
  ["@@INSTALLER_SHA256@@", installerSha256],
]);

function renderTemplate(fileName) {
  let output = readFileSync(join(templateRoot, fileName), "utf8");
  for (const [token, value] of replacements.entries()) {
    output = output.replaceAll(token, value);
  }
  return output;
}

writeFileSync(
  join(manifestDir, `${packageIdentifier}.yaml`),
  renderTemplate("version.yaml"),
);
writeFileSync(
  join(manifestDir, `${packageIdentifier}.locale.en-US.yaml`),
  renderTemplate("defaultLocale.yaml"),
);
writeFileSync(
  join(manifestDir, `${packageIdentifier}.installer.yaml`),
  renderTemplate("installer.yaml"),
);

process.stdout.write(`${manifestDir}\n`);
