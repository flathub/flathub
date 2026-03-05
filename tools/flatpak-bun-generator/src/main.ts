import { readFileSync, writeFileSync } from "fs";
import { base64ToHex, splitOnce, stripJsoncTrailingCommas } from "./utils.ts";
import { bunCacheVersion } from "./wyhash.ts";

export interface BunPackage {
  identifier: string;
  name: string;
  version: string;
  integrity: string;
  os?: string;
  cpu?: string;
}

export interface GitBunPackage {
  identifier: string;
  owner: string;
  repo: string;
  commit: string;
}

export interface FlatpakSource {
  type: string;
  url?: string;
  dest: string;
  "dest-filename"?: string;
  "only-arches"?: string[];
  "strip-components"?: number;
  sha256?: string;
  sha512?: string;
}

export interface ElectronInfo {
  /** Full version string from package.json, e.g. "40.1.0+wvcus" */
  fullVersion: string;
  /** Base semver version without build metadata, e.g. "40.1.0" */
  baseVersion: string;
  /** Build metadata suffix, e.g. "wvcus" (or null for stock Electron) */
  buildMeta: string | null;
  /** Whether this is a castlabs fork (has +wvcus or similar suffix) */
  isCastlabs: boolean;
  /** The git commit from the lockfile */
  commit: string;
  /** GitHub owner */
  owner: string;
  /** GitHub repo */
  repo: string;
}

export function parseBunLockfile(text: string): {
  lockfileVersion: number;
  packages: Record<string, any[]>;
  workspaces: Record<string, any>;
} {
  const cleaned = stripJsoncTrailingCommas(text);
  const data = JSON.parse(cleaned);

  if (data.lockfileVersion !== 1) {
    throw new Error(
      `Unsupported bun lockfile version: ${data.lockfileVersion}. Only version 1 is supported.`
    );
  }

  return data;
}

export function extractPackages(
  packagesMap: Record<string, any[]>,
  options: { allOs: boolean; noDev: boolean; devPackageNames: Set<string> }
): BunPackage[] {
  const packages: BunPackage[] = [];
  const seen = new Set<string>();

  for (const [key, entry] of Object.entries(packagesMap)) {
    if (!Array.isArray(entry) || entry.length < 4) continue;

    const identifier: string = entry[0];

    if (!identifier) continue;

    if (typeof entry[1] !== "string") {
      continue;
    }

    const meta: Record<string, any> = entry[2] ?? {};
    const integrity: string = entry[3] ?? "";

    if (!integrity) continue;

    if (seen.has(identifier)) continue;
    seen.add(identifier);

    const parsed = parseIdentifier(identifier);
    if (!parsed) continue;

    const { name, version } = parsed;

    if (options.noDev && options.devPackageNames.has(key)) {
      continue;
    }

    const os = meta.os as string | undefined;
    if (!options.allOs && os !== undefined && os !== "linux") {
      continue;
    }

    const cpu = meta.cpu as string | undefined;

    packages.push({
      identifier,
      name,
      version,
      integrity,
      os,
      cpu,
    });
  }

  return packages;
}

export function parseIdentifier(
  identifier: string
): { name: string; version: string } | null {
  const atIdx = identifier.lastIndexOf("@");
  if (atIdx <= 0) return null;

  const name = identifier.slice(0, atIdx);
  const version = identifier.slice(atIdx + 1);

  if (!name || !version) return null;
  return { name, version };
}

export function parseGitIdentifier(
  identifier: string
): { owner: string; repo: string; commit: string } | null {
  const match = identifier.match(/@github:([^/]+)\/([^#]+)#(.+)$/);
  if (!match) return null;

  const [, owner, repo, commit] = match;
  if (!owner || !repo || !commit) return null;

  return { owner, repo, commit };
}

export function extractGitPackages(
  packagesMap: Record<string, any[]>
): GitBunPackage[] {
  const packages: GitBunPackage[] = [];
  const seen = new Set<string>();

  for (const [_key, entry] of Object.entries(packagesMap)) {
    if (!Array.isArray(entry) || entry.length < 3) continue;

    const identifier: string = entry[0];
    if (!identifier) continue;

    if (typeof entry[1] === "string") continue;

    if (seen.has(identifier)) continue;
    seen.add(identifier);

    const parsed = parseGitIdentifier(identifier);
    if (!parsed) {
      console.warn(
        `Skipping ${identifier}: unable to parse git dependency identifier. ` +
          `Only github: dependencies are supported.`
      );
      continue;
    }

    packages.push({
      identifier,
      ...parsed,
    });
  }

  return packages;
}

function mapCpuToArch(cpu: string): string | null {
  switch (cpu) {
    case "x64":
      return "x86_64";
    case "arm64":
      return "aarch64";
    default:
      return null;
  }
}

export function npmPkgToFlatpakSources(
  pkg: BunPackage,
  registry: string
): FlatpakSource[] {
  const [checksumType, checksumValue] = splitOnce(pkg.integrity, "-");
  if (!checksumValue) {
    console.warn(
      `Skipping ${pkg.name}@${pkg.version}: unable to parse integrity hash`
    );
    return [];
  }

  const basename = pkg.name.startsWith("@")
    ? pkg.name.split("/")[1]
    : pkg.name;
  const tarballUrl = `${registry}/${pkg.name}/-/${basename}-${pkg.version}.tgz`;

  const cacheVersion = bunCacheVersion(pkg.version);

  const hexChecksum = base64ToHex(checksumValue);
  const fileSource: FlatpakSource = {
    type: "file",
    url: tarballUrl,
    [checksumType]: hexChecksum,
    dest: "bun_cache",
    "dest-filename": `${pkg.name.replace("/", "--")}@${cacheVersion}.tgz`,
  };

  if (pkg.cpu) {
    const arch = mapCpuToArch(pkg.cpu);
    if (arch) {
      fileSource["only-arches"] = [arch];
    }
  }

  return [fileSource];
}

export function gitPkgToFlatpakSource(
  pkg: GitBunPackage,
  sha256Hash: string
): FlatpakSource {
  const url = `https://github.com/${pkg.owner}/${pkg.repo}/archive/${pkg.commit}.tar.gz`;
  const dest = `bun_cache/@GH@${pkg.owner}-${pkg.repo}-${pkg.commit}@@@1`;

  return {
    type: "archive",
    url,
    sha256: sha256Hash,
    dest,
    "strip-components": 1,
  };
}

export async function fetchSha256(url: string): Promise<string> {
  const response = await fetch(url, { redirect: "follow" });
  if (!response.ok) {
    throw new Error(
      `Failed to fetch ${url}: ${response.status} ${response.statusText}`
    );
  }
  const data = await response.arrayBuffer();
  const hashBuffer = await crypto.subtle.digest("SHA-256", data);
  return Array.from(new Uint8Array(hashBuffer))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

export function collectDevDependencyNames(
  workspaces: Record<string, any>,
  packagesMap: Record<string, any[]>
): Set<string> {
  const devRoots = new Set<string>();
  const prodRoots = new Set<string>();

  for (const ws of Object.values(workspaces)) {
    if (ws.dependencies) {
      for (const name of Object.keys(ws.dependencies)) {
        prodRoots.add(name);
      }
    }
    if (ws.devDependencies) {
      for (const name of Object.keys(ws.devDependencies)) {
        devRoots.add(name);
      }
    }
    if (ws.optionalDependencies) {
      for (const name of Object.keys(ws.optionalDependencies)) {
        prodRoots.add(name);
      }
    }
    if (ws.peerDependencies) {
      for (const name of Object.keys(ws.peerDependencies)) {
        prodRoots.add(name);
      }
    }
  }

  function resolveDep(parentKey: string, depName: string): string | null {
    const nestedKey = `${parentKey}/${depName}`;
    if (nestedKey in packagesMap) return nestedKey;
    if (depName in packagesMap) return depName;
    return null;
  }

  const prodReachable = new Set<string>();
  const queue: string[] = [];

  for (const name of prodRoots) {
    if (name in packagesMap) {
      queue.push(name);
      prodReachable.add(name);
    }
  }

  while (queue.length > 0) {
    const key = queue.pop()!;
    const entry = packagesMap[key];
    if (!Array.isArray(entry) || entry.length < 3) continue;

    const meta = entry[2] ?? {};
    const deps = {
      ...meta.dependencies,
      ...meta.optionalDependencies,
      ...meta.peerDependencies,
    };

    for (const depName of Object.keys(deps)) {
      const resolvedKey = resolveDep(key, depName);
      if (resolvedKey && !prodReachable.has(resolvedKey)) {
        prodReachable.add(resolvedKey);
        queue.push(resolvedKey);
      }
    }
  }

  const devOnly = new Set<string>();
  for (const key of Object.keys(packagesMap)) {
    if (!prodReachable.has(key)) {
      devOnly.add(key);
    }
  }

  return devOnly;
}

// ---------------------------------------------------------------------------
// Electron binary & node-headers source generation
// ---------------------------------------------------------------------------

const ELECTRON_ARCHES: { flatpak: string; electron: string }[] = [
  { flatpak: "x86_64", electron: "x64" },
  { flatpak: "aarch64", electron: "arm64" },
];

/**
 * Detect whether any of the git dependencies is an Electron fork
 * (currently: castlabs/electron-releases). Returns the matching package
 * or null.
 */
export function detectElectronGitDep(
  gitPackages: GitBunPackage[]
): GitBunPackage | null {
  return (
    gitPackages.find(
      (pkg) =>
        pkg.repo === "electron-releases" && pkg.owner === "castlabs"
    ) ?? null
  );
}

/**
 * Fetch the package.json from a GitHub commit and extract the version field.
 * Returns the raw version string (e.g. "40.1.0+wvcus").
 */
export async function getElectronVersion(
  owner: string,
  repo: string,
  commit: string
): Promise<string> {
  const url = `https://raw.githubusercontent.com/${owner}/${repo}/${commit}/package.json`;
  const response = await fetch(url, { redirect: "follow" });
  if (!response.ok) {
    throw new Error(
      `Failed to fetch ${url}: ${response.status} ${response.statusText}`
    );
  }
  const pkg = (await response.json()) as { version?: string };
  if (!pkg.version) {
    throw new Error(
      `No "version" field found in ${owner}/${repo}@${commit}/package.json`
    );
  }
  return pkg.version;
}

/**
 * Parse the full electron version into its components.
 * "40.1.0+wvcus" -> { fullVersion: "40.1.0+wvcus", baseVersion: "40.1.0", buildMeta: "wvcus", ... }
 */
export function parseElectronVersion(
  fullVersion: string,
  gitPkg: GitBunPackage
): ElectronInfo {
  const plusIdx = fullVersion.indexOf("+");
  const baseVersion = plusIdx !== -1 ? fullVersion.slice(0, plusIdx) : fullVersion;
  const buildMeta = plusIdx !== -1 ? fullVersion.slice(plusIdx + 1) : null;

  return {
    fullVersion,
    baseVersion,
    buildMeta,
    isCastlabs: gitPkg.owner === "castlabs",
    commit: gitPkg.commit,
    owner: gitPkg.owner,
    repo: gitPkg.repo,
  };
}

/**
 * Compute the @electron/get cache directory hash.
 *
 * @electron/get uses:
 *   1. Parse the download URL with Node's legacy url.parse()
 *   2. Strip the filename (path.dirname on pathname), strip search & hash
 *   3. Re-format with url.format()
 *   4. SHA-256 hex of the result
 *
 * For castlabs: the download URL directory is
 *   https://github.com/castlabs/electron-releases/releases/download/v{version}
 * where {version} contains a literal "+" (not %2B).
 */
export async function computeElectronCacheKey(
  downloadDirUrl: string
): Promise<string> {
  const data = new TextEncoder().encode(downloadDirUrl);
  const hashBuffer = await crypto.subtle.digest("SHA-256", data);
  return Array.from(new Uint8Array(hashBuffer))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

/**
 * Build the download directory URL that @electron/get would compute
 * for a given electron version from castlabs.
 */
export function electronDownloadDirUrl(info: ElectronInfo): string {
  const versionTag = info.buildMeta
    ? `v${info.baseVersion}+${info.buildMeta}`
    : `v${info.baseVersion}`;
  return `https://github.com/${info.owner}/${info.repo}/releases/download/${versionTag}`;
}

/**
 * Build the download URL for the electron binary zip from castlabs.
 * The tag portion of the URL must use %2B for "+".
 */
export function electronBinaryUrl(
  info: ElectronInfo,
  electronArch: string
): string {
  const versionTag = info.buildMeta
    ? `v${info.baseVersion}%2B${info.buildMeta}`
    : `v${info.baseVersion}`;
  const filename = info.buildMeta
    ? `electron-v${info.baseVersion}+${info.buildMeta}-linux-${electronArch}.zip`
    : `electron-v${info.baseVersion}-linux-${electronArch}.zip`;
  return `https://github.com/${info.owner}/${info.repo}/releases/download/${versionTag}/${filename}`;
}

/**
 * Generate FlatpakSource entries for the Electron binary zip.
 * One per architecture (x64 -> x86_64, arm64 -> aarch64).
 * Architectures where the binary is not available (404) are skipped.
 */
export async function electronBinarySources(
  info: ElectronInfo
): Promise<FlatpakSource[]> {
  const dirUrl = electronDownloadDirUrl(info);
  const cacheKey = await computeElectronCacheKey(dirUrl);

  const sources: FlatpakSource[] = [];
  for (const arch of ELECTRON_ARCHES) {
    const url = electronBinaryUrl(info, arch.electron);
    const filename = info.buildMeta
      ? `electron-v${info.baseVersion}+${info.buildMeta}-linux-${arch.electron}.zip`
      : `electron-v${info.baseVersion}-linux-${arch.electron}.zip`;

    try {
      const sha256 = await fetchSha256(url);
      sources.push({
        type: "file",
        url,
        sha256,
        dest: `electron-cache/${cacheKey}`,
        "dest-filename": filename,
        "only-arches": [arch.flatpak],
      });
    } catch (_err) {
      console.warn(
        `    Skipping ${filename}: binary not available for ${arch.electron}`
      );
    }
  }

  return sources;
}

/**
 * Build the URL for node headers.
 * Node headers come from artifacts.electronjs.org using the base version
 * (without build metadata like +wvcus).
 */
export function nodeHeadersUrl(info: ElectronInfo): string {
  return `https://artifacts.electronjs.org/headers/dist/v${info.baseVersion}/node-v${info.baseVersion}-headers.tar.gz`;
}

/**
 * Generate the FlatpakSource for node headers.
 */
export async function nodeHeadersSource(
  info: ElectronInfo
): Promise<FlatpakSource> {
  const url = nodeHeadersUrl(info);
  const sha256 = await fetchSha256(url);

  return {
    type: "archive",
    url,
    sha256,
    dest: "electron-headers",
    "strip-components": 1,
  };
}

/**
 * Given the list of git packages from the lockfile, detect if there is a
 * castlabs Electron dependency. If so, fetch its version, and generate
 * the electron binary zip + node headers sources.
 *
 * Returns the generated sources (empty array if no Electron dep found).
 */
export async function generateElectronSources(
  gitPackages: GitBunPackage[]
): Promise<FlatpakSource[]> {
  const electronDep = detectElectronGitDep(gitPackages);
  if (!electronDep) return [];

  console.log(
    `Detected Electron git dependency: ${electronDep.owner}/${electronDep.repo}@${electronDep.commit}`
  );

  const fullVersion = await getElectronVersion(
    electronDep.owner,
    electronDep.repo,
    electronDep.commit
  );
  const info = parseElectronVersion(fullVersion, electronDep);

  console.log(
    `  Electron version: ${info.fullVersion} (base: ${info.baseVersion})`
  );

  const sources: FlatpakSource[] = [];

  console.log(`  Fetching electron binary zip hashes...`);
  const binarySources = await electronBinarySources(info);
  sources.push(...binarySources);
  for (const src of binarySources) {
    console.log(`    ${src["dest-filename"]} (${src["only-arches"]}) OK`);
  }

  console.log(`  Fetching node headers hash...`);
  const headers = await nodeHeadersSource(info);
  sources.push(headers);
  console.log(`    node-v${info.baseVersion}-headers.tar.gz OK`);

  return sources;
}

export async function main(
  lockPath: string,
  outputPath: string = "generated-sources.json",
  options: {
    allOs?: boolean;
    noDev?: boolean;
    registry?: string;
  } = {}
): Promise<void> {
  const { allOs = false, noDev = false, registry = "https://registry.npmjs.org" } = options;
  const registryUrl = registry.replace(/\/$/, "");

  const lockText = readFileSync(lockPath, "utf-8");
  const lock = parseBunLockfile(lockText);

  const devPackageNames = noDev
    ? collectDevDependencyNames(lock.workspaces, lock.packages)
    : new Set<string>();

  const packages = extractPackages(lock.packages, {
    allOs,
    noDev,
    devPackageNames,
  });

  console.log(`Processing ${packages.length} packages from ${lockPath}...`);

  const sourceArrays = packages.map((pkg) =>
    npmPkgToFlatpakSources(pkg, registryUrl)
  );

  const flatpakSources: FlatpakSource[] = sourceArrays.flat();

  const gitPackages = extractGitPackages(lock.packages);
  if (gitPackages.length > 0) {
    console.log(
      `Fetching hashes for ${gitPackages.length} git dependencies...`
    );
    for (const gitPkg of gitPackages) {
      const url = `https://github.com/${gitPkg.owner}/${gitPkg.repo}/archive/${gitPkg.commit}.tar.gz`;
      try {
        const hash = await fetchSha256(url);
        flatpakSources.push(gitPkgToFlatpakSource(gitPkg, hash));
        console.log(`  ${gitPkg.owner}/${gitPkg.repo}@${gitPkg.commit} OK`);
      } catch (err: any) {
        console.error(
          `  Failed to fetch hash for ${gitPkg.owner}/${gitPkg.repo}@${gitPkg.commit}: ${err.message}`
        );
      }
    }
  }

  // Detect castlabs Electron git dependency and generate binary + headers sources
  const electronSources = await generateElectronSources(gitPackages);
  flatpakSources.push(...electronSources);

  writeFileSync(outputPath, JSON.stringify(flatpakSources, null, 2) + "\n");
  const electronCount = electronSources.length;
  console.log(
    `Wrote ${flatpakSources.length} sources (${packages.length} npm + ${gitPackages.length} git + ${electronCount} electron) to ${outputPath}`
  );
}

if (import.meta.main || process.argv[1]?.endsWith("main.ts")) {
  const args = process.argv.slice(2);

  const lockPath = args.find((a) => !a.startsWith("--"));
  if (!lockPath) {
    console.error(
      "Usage: bun run src/main.ts <path-to-bun.lock> [--output <file>] [--all-os] [--no-devel] [--registry <url>]"
    );
    process.exit(1);
  }

  const outputIdx = args.indexOf("--output");
  const outputPath =
    outputIdx !== -1 ? args[outputIdx + 1] : "generated-sources.json";

  const allOs = args.includes("--all-os");
  const noDev = args.includes("--no-devel");

  const registryIdx = args.indexOf("--registry");
  const registry =
    registryIdx !== -1
      ? args[registryIdx + 1]
      : "https://registry.npmjs.org";

  main(lockPath, outputPath, { allOs, noDev, registry }).catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
