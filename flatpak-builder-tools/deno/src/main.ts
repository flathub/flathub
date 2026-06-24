// LICENSE = MIT
import {
  base64ToHex,
  sha256,
  shortHash,
  shouldHash,
  splitOnce,
  urlSegments,
} from "./utils.ts";

/**
 * Represents a JSR or NPM package.
 */
export interface Pkg {
  /** The module identifier (e.g., scope/name for JSR, or package name for NPM). */
  module: string;
  /** The specific version of the package. */
  version: string;
  /** The short name of the package. */
  name: string;
  /** Optional CPU architecture for which this package is intended. */
  cpu?: "x86_64" | "aarch64";
}

/**
 * Represents a source entry in a Flatpak manifest.
 */
export interface FlatpakData {
  /** The type of the source, e.g., "file", "archive". */
  type: string;
  /** The URL from which to download the source. */
  url?: string;
  /**
   * Optional inline contents for the source.
   * Used when the source is provided directly rather than downloaded from a URL.
   */
  contents?: string;
  /** The destination directory within the build environment where the source will be placed. */
  dest: string;
  /** Optional name for the downloaded file. If not provided, the name is derived from the URL. */
  "dest-filename"?: string;
  /** Optional array of CPU architectures for which this source is relevant. */
  "only-arches"?: ("x86_64" | "aarch64")[];
  /**
   * Optional type of the archive, if the source is an archive.
   * This helps Flatpak to correctly extract the contents.
   */
  "archive-type"?:
    | "tar-gzip"
    | "rpm"
    | "tar"
    | "tar-gzip"
    | "tar-compress"
    | "tar-bzip2"
    | "tar-lzip"
    | "tar-lzma"
    | "tar-lzop"
    | "tar-xz"
    | "tar-zst"
    | "zip"
    | "7z";
  /** Optional SHA256 checksum for verifying the integrity of the downloaded source. */
  sha256?: string;
  /** Optional SHA512 checksum for verifying the integrity of the downloaded source. */
  sha512?: string;
}

/**
 * Converts JSR package information into an array of FlatpakData objects.
 * It fetches metadata for the package and its specific version, then processes
 * the module graph to create download entries for each file.
 * @param pkg The JSR package information.
 * @returns A promise that resolves to an array of FlatpakData objects.
 */
export async function jsrPkgToFlatpakData(pkg: Pkg): Promise<FlatpakData[]> {
  const flatpkData: FlatpakData[] = [];
  const metaUrl = `https://jsr.io/${pkg.module}/meta.json`;
  const metaJson = await fetch(
    metaUrl,
  ).then((r) => r.json());

  // "meta.json" file is a stateful file, thats why we inline what we need
  flatpkData.push({
    type: "inline",
    contents: JSON.stringify({
      scope: metaJson.scope,
      name: metaJson.name,
      latest: metaJson.latest,
      versions: {},
    }),
    dest: `vendor/jsr.io/${pkg.module}`,
    "dest-filename": "meta.json",
  });

  const metaVerUrl = `https://jsr.io/${pkg.module}/${pkg.version}_meta.json`;
  const metaVerText = await fetch(
    metaVerUrl,
  ).then((r) => r.text());

  flatpkData.push({
    type: "file",
    url: metaVerUrl,
    sha256: await sha256(metaVerText),
    dest: `vendor/jsr.io/${pkg.module}`,
    "dest-filename": `${pkg.version}_meta.json`,
  });

  const metaVer = JSON.parse(metaVerText);

  for (
    const fileUrl of Object.keys(metaVer.moduleGraph2 || metaVer.moduleGraph1)
  ) {
    const fileMeta = metaVer.manifest[fileUrl];
    // this mean the url exists in the module graph but not in the manifest -> this url is not needed
    if (!fileMeta) continue;
    const [checksumType, checksumValue] = splitOnce(fileMeta.checksum, "-");

    const url = `https://jsr.io/${pkg.module}/${pkg.version}${fileUrl}`;
    const segments = fileUrl.split("/").filter(Boolean);
    const hashedSegments = await Promise.all(
      segments.map(async (part) =>
        shouldHash(part) ? await shortHash(part) : part
      ),
    );
    const fileName = hashedSegments.pop();
    const dest = `vendor/jsr.io/${pkg.module}/${pkg.version}${
      hashedSegments.length > 0 ? "/" + hashedSegments.join("/") : ""
    }`;

    flatpkData.push({
      type: "file",
      url,
      [checksumType]: checksumValue,
      dest,
      "dest-filename": fileName,
    });
  }

  // If a moule imports deno.json (import ... from "deno.json" with {type:"json"}), it won't appear in the module graph
  // Worarkound: if there is a deno.json file in the manifest just add it
  // Note this can be made better, by looking in the moduleGraph if deno.json is specified in the dependencies
  for (const [fileUrl, fileMeta] of Object.entries(metaVer.manifest)) {
    if (fileUrl.includes("deno.json")) {
      const [checksumType, checksumValue] = splitOnce(
        // deno-lint-ignore no-explicit-any
        (fileMeta as any).checksum,
        "-",
      );
      const url = `https://jsr.io/${pkg.module}/${pkg.version}${fileUrl}`;
      const segments = fileUrl.split("/").filter(Boolean);
      const hashedSegments = await Promise.all(
        segments.map(async (part) =>
          shouldHash(part) ? await shortHash(part) : part
        ),
      );
      const fileName = hashedSegments.pop();
      const dest = `vendor/jsr.io/${pkg.module}/${pkg.version}${
        hashedSegments.length > 0 ? "/" + hashedSegments.join("/") : ""
      }`;

      flatpkData.push({
        type: "file",
        url,
        [checksumType]: checksumValue,
        dest,
        "dest-filename": fileName,
      });
    }
  }

  return flatpkData;
}

/**
 * Checks if a package is a JSR package accessed via npm.jsr.io
 * @param module The module identifier (e.g., "@jsr/sigma__deno-compat")
 * @returns true if this is a JSR package via npm compatibility layer
 */
function isJsrNpmPackage(module: string): boolean {
  return module.startsWith("@jsr/");
}

/**
 * Converts JSR package (accessed via npm.jsr.io) into FlatpakData objects.
 * JSR packages via npm have tarball URLs and integrity in the lock file,
 * so we don't need to fetch from a registry.
 * @param pkg The JSR-via-npm package information.
 * @param lockData The lock file data for this specific package.
 * @returns A promise that resolves to an array of FlatpakData objects.
 */
export function jsrNpmPkgToFlatpakData(
  pkg: Pkg,
  // deno-lint-ignore no-explicit-any
  lockData: any,
): FlatpakData[] {
  const tarballUrl = lockData.tarball;
  const integrity = lockData.integrity;

  if (!tarballUrl || !integrity) {
    throw new Error(
      `Missing tarball or integrity for JSR package ${pkg.module}@${pkg.version}`,
    );
  }

  const [checksumType, checksumValue] = splitOnce(integrity, "-");

  const registryContents = JSON.stringify({
    name: pkg.module,
    "dist-tags": {},
    versions: {
      [pkg.version]: {
        dist: {
          integrity: integrity,
          tarball: tarballUrl,
        },
      },
    },
  });

  // Deno's npm cache resolution for JSR packages can be inconsistent
  // Sometimes it looks under npm.jsr.io, sometimes under registry.npmjs.org
  // We generate entries for both paths to ensure compatibility
  const results: FlatpakData[] = [];

  // Create inline registry.json for npm.jsr.io path
  results.push({
    type: "inline",
    contents: registryContents,
    dest: `deno_dir/npm/npm.jsr.io/${pkg.module}`,
    "dest-filename": "registry.json",
  });

  // Create inline registry.json for registry.npmjs.org path
  results.push({
    type: "inline",
    contents: registryContents,
    dest: `deno_dir/npm/registry.npmjs.org/${pkg.module}`,
    "dest-filename": "registry.json",
  });

  // Archive for npm.jsr.io path
  const pkgDataJsr: FlatpakData = {
    type: "archive",
    "archive-type": "tar-gzip",
    url: tarballUrl,
    [checksumType]: base64ToHex(checksumValue),
    dest: `deno_dir/npm/npm.jsr.io/${pkg.module}/${pkg.version}`,
  };

  if (pkg.cpu) {
    pkgDataJsr["only-arches"] = [pkg.cpu];
  }

  results.push(pkgDataJsr);

  // Archive for registry.npmjs.org path
  const pkgDataNpm: FlatpakData = {
    type: "archive",
    "archive-type": "tar-gzip",
    url: tarballUrl,
    [checksumType]: base64ToHex(checksumValue),
    dest: `deno_dir/npm/registry.npmjs.org/${pkg.module}/${pkg.version}`,
  };

  if (pkg.cpu) {
    pkgDataNpm["only-arches"] = [pkg.cpu];
  }

  results.push(pkgDataNpm);

  return results;
}

/**
 * Converts NPM package information into an array of FlatpakData objects.
 * It fetches metadata for the package and creates entries for the package's
 * registry metadata and the package tarball itself.
 * @param pkg The NPM package information.
 * @returns A promise that resolves to an array of FlatpakData objects.
 */
export async function npmPkgToFlatpakData(pkg: Pkg): Promise<FlatpakData[]> {
  //url: https://registry.npmjs.org/@napi-rs/cli/-/cli-2.18.4.tgz
  //npmPkgs;
  const metaUrl = `https://registry.npmjs.org/${pkg.module}`;
  const meta = await fetch(metaUrl).then(
    (r) => r.json(),
  );

  // "registry.json" file is a stateful file, its always updated so it will never have the same hash for ever
  // the workaround is to snapshot it by inlining its content as a string, though we do some optimization here to reduce its size
  // by only taking the necessary fields
  const metaData = {
    type: "inline",
    contents: JSON.stringify({
      name: meta.name,
      "dist-tags": {},
      versions: { [pkg.version]: meta.versions[pkg.version] },
    }),
    dest: `deno_dir/npm/registry.npmjs.org/${pkg.module}`,
    "dest-filename": "registry.json",
  };

  const [checksumType, checksumValue] = splitOnce(
    meta.versions[pkg.version].dist.integrity,
    "-",
  );
  const pkgData: FlatpakData = {
    type: "archive",
    "archive-type": "tar-gzip",
    url:
      `https://registry.npmjs.org/${pkg.module}/-/${pkg.name}-${pkg.version}.tgz`,
    [checksumType]: base64ToHex(checksumValue),
    dest: `deno_dir/npm/registry.npmjs.org/${pkg.module}/${pkg.version}`,
  };

  if (pkg.cpu) {
    pkgData["only-arches"] = [pkg.cpu];
  }

  return [metaData, pkgData];
}

/**
 * Main function to generate Flatpak sources from a Deno lock file.
 * It reads the lock file, processes JSR, NPM, and remote HTTP dependencies,
 * and writes the resulting FlatpakData array to an output JSON file.
 * @param lockPath Path to the Deno lock file.
 * @param outputPath Path to the output JSON file for Flatpak sources. Defaults to "deno-sources.json".
 * @param allOs If true, include all packages regardless of target OS. Defaults to false.
 */
export async function main(
  lockPath: string,
  outputPath: string = "deno-sources.json",
  allOs: boolean = false,
) {
  const lock = JSON.parse(Deno.readTextFileSync(lockPath));
  if (lock.version !== "5") {
    throw new Error(`Unsupported deno lock version: ${lock.version}`);
  }

  const jsrPkgs: Pkg[] = !lock.jsr ? [] : Object.keys(lock.jsr).map((pkg) => {
    const r = splitOnce(pkg, "@", "right");
    const name = r[0].split("/")[1];
    return { module: r[0], version: r[1], name };
  });
  jsrPkgs;

  // Process npm packages, separating JSR packages from regular npm packages
  // deno-lint-ignore no-explicit-any
  const npmEntries: Array<[string, any]> = !lock.npm
    ? []
    : Object.entries(lock.npm)
      .filter((
        // deno-lint-ignore no-explicit-any
        [_key, val]: any,
      ) => (allOs || val.os === undefined || val.os?.at(0) === "linux"));

  // deno-lint-ignore no-explicit-any
  const npmPkgs: Array<[Pkg, any]> = npmEntries
    .filter(([key, _val]) => {
      const r = key.match(/(^@?.+?)@([^_]+?)(?=_|$)/)!;
      return !isJsrNpmPackage(r[1]);
    })
    // deno-lint-ignore no-explicit-any
    .map(([key, val]: [string, any]) => {
      const r = key.match(/(^@?.+?)@([^_]+?)(?=_|$)/)!;
      const name = r[1].includes("/") ? r[1].split("/")[1] : r[1];
      const cpu = val.cpu?.at(0);
      return [
        {
          module: r[1],
          version: r[2],
          name,
          cpu: cpu === "x64" ? "x86_64" : cpu === "arm64" ? "aarch64" : cpu,
        },
        val,
      ];
    });

  // deno-lint-ignore no-explicit-any
  const jsrNpmPkgs: Array<[Pkg, any]> = npmEntries
    .filter(([key, _val]) => {
      const r = key.match(/(^@?.+?)@([^_]+?)(?=_|$)/)!;
      return isJsrNpmPackage(r[1]);
    })
    // deno-lint-ignore no-explicit-any
    .map(([key, val]: [string, any]) => {
      const r = key.match(/(^@?.+?)@([^_]+?)(?=_|$)/)!;
      const name = r[1].includes("/") ? r[1].split("/")[1] : r[1];
      const cpu = val.cpu?.at(0);
      return [
        {
          module: r[1],
          version: r[2],
          name,
          cpu: cpu === "x64" ? "x86_64" : cpu === "arm64" ? "aarch64" : cpu,
        },
        val,
      ];
    });
  const httpPkgsData = !lock.remote
    ? []
    : Object.entries(lock.remote).map(async ([urlStr, checksum]) => {
      const url = new URL(urlStr);
      const segments = await Promise.all(
        urlSegments(url)
          .map(async (part) => shouldHash(part) ? await shortHash(part) : part),
      );
      const filename = segments.pop();
      return {
        type: "file",
        url: urlStr,
        sha256: checksum,
        dest: `vendor/${url.hostname}/${segments.join("/")}`,
        "dest-filename": filename,
      };
    });

  const flatpakData = [
    await Promise.all(
      jsrPkgs.map((pkg) => jsrPkgToFlatpakData(pkg)),
    ).then((r) => r.flat()),
    await Promise.all(
      npmPkgs.map(([pkg, _lockData]) => npmPkgToFlatpakData(pkg)),
    ).then((r) => r.flat()),
    await Promise.all(
      jsrNpmPkgs.map(([pkg, lockData]) =>
        jsrNpmPkgToFlatpakData(pkg, lockData)
      ),
    ).then((r) => r.flat()),
    await Promise.all(httpPkgsData),
  ].flat();
  // console.log(flatpakData);
  Deno.writeTextFileSync(
    outputPath,
    JSON.stringify(flatpakData, null, 2),
  );
}

if (import.meta.main) {
  const args = Deno.args;
  const lockPath = args[0];
  if (!lockPath) {
    console.error(
      "Usage: deno run -RN -W=. <this_script> <path-to-lock-file> [--output <output-file>] [--all-os]",
    );
    console.error(
      `Examples:
     - deno run -RN -W=. main.ts deno.lock
     - deno run -RN -W=. main.ts deno.lock --all-os
     - deno run -RN -W=. jsr:@flatpak-contrib/flatpak-deno-generator deno.lock --output sources.json`,
    );
    Deno.exit(1);
  }
  const outputFile = args.includes("--output")
    ? args[args.indexOf("--output") + 1]
    : "deno-sources.json";
  const allOs = args.includes("--all-os");
  await main(lockPath, outputFile, allOs);
}
