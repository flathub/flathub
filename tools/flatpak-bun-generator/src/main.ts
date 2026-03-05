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

  writeFileSync(outputPath, JSON.stringify(flatpakSources, null, 2) + "\n");
  console.log(
    `Wrote ${flatpakSources.length} sources (${packages.length} npm + ${gitPackages.length} git) to ${outputPath}`
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
