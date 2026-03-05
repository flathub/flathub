import { describe, expect, test } from "bun:test";
import {
  parseBunLockfile,
  extractPackages,
  extractGitPackages,
  parseIdentifier,
  parseGitIdentifier,
  npmPkgToFlatpakSources,
  gitPkgToFlatpakSource,
  collectDevDependencyNames,
} from "../src/main.ts";

describe("parseIdentifier", () => {
  test("parses regular package", () => {
    expect(parseIdentifier("lodash@4.17.21")).toEqual({
      name: "lodash",
      version: "4.17.21",
    });
  });

  test("parses scoped package", () => {
    expect(parseIdentifier("@babel/core@7.29.0")).toEqual({
      name: "@babel/core",
      version: "7.29.0",
    });
  });

  test("returns null for invalid input", () => {
    expect(parseIdentifier("invalid")).toBeNull();
  });

  test("returns null for empty string", () => {
    expect(parseIdentifier("")).toBeNull();
  });
});

describe("parseBunLockfile", () => {
  test("parses valid bun.lock content", () => {
    const content = `{
      "lockfileVersion": 1,
      "configVersion": 1,
      "workspaces": {
        "": {
          "name": "test",
          "dependencies": {
            "lodash": "^4.17.21",
          },
        },
      },
      "packages": {
        "lodash": ["lodash@4.17.23", "", {}, "sha512-abc123=="],
      }
    }`;

    const result = parseBunLockfile(content);
    expect(result.lockfileVersion).toBe(1);
    expect(result.packages["lodash"]).toBeDefined();
    expect(result.packages["lodash"][0]).toBe("lodash@4.17.23");
  });

  test("throws for unsupported lockfile version", () => {
    const content = '{"lockfileVersion": 99, "packages": {}}';
    expect(() => parseBunLockfile(content)).toThrow(
      "Unsupported bun lockfile version: 99"
    );
  });
});

describe("extractPackages", () => {
  const samplePackages: Record<string, any[]> = {
    lodash: [
      "lodash@4.17.23",
      "",
      {},
      "sha512-abc123==",
    ],
    "@types/node": [
      "@types/node@20.19.35",
      "",
      { dependencies: { "undici-types": "~6.21.0" } },
      "sha512-xyz789==",
    ],
    "@rollup/rollup-linux-x64-gnu": [
      "@rollup/rollup-linux-x64-gnu@4.59.0",
      "",
      { os: "linux", cpu: "x64" },
      "sha512-platform==",
    ],
    "@rollup/rollup-darwin-arm64": [
      "@rollup/rollup-darwin-arm64@4.59.0",
      "",
      { os: "darwin", cpu: "arm64" },
      "sha512-darwinpkg==",
    ],
  };

  test("extracts all packages when allOs is true", () => {
    const result = extractPackages(samplePackages, {
      allOs: true,
      noDev: false,
      devPackageNames: new Set(),
    });
    expect(result).toHaveLength(4);
  });

  test("filters non-linux packages when allOs is false", () => {
    const result = extractPackages(samplePackages, {
      allOs: false,
      noDev: false,
      devPackageNames: new Set(),
    });
    expect(result).toHaveLength(3);
    expect(result.find((p) => p.name === "@rollup/rollup-darwin-arm64")).toBeUndefined();
  });

  test("filters dev dependencies when noDev is true", () => {
    const result = extractPackages(samplePackages, {
      allOs: true,
      noDev: true,
      devPackageNames: new Set(["@types/node"]),
    });
    expect(result).toHaveLength(3);
    expect(result.find((p) => p.name === "@types/node")).toBeUndefined();
  });

  test("parses CPU field correctly", () => {
    const result = extractPackages(samplePackages, {
      allOs: true,
      noDev: false,
      devPackageNames: new Set(),
    });
    const linuxPkg = result.find(
      (p) => p.name === "@rollup/rollup-linux-x64-gnu"
    );
    expect(linuxPkg?.cpu).toBe("x64");
  });

  test("deduplicates nested dependencies with same identifier", () => {
    const packagesWithDupes: Record<string, any[]> = {
      debug: ["debug@4.4.3", "", {}, "sha512-aaaa=="],
      "body-parser/debug": ["debug@2.6.9", "", {}, "sha512-bbbb=="],
      "express/debug": ["debug@2.6.9", "", {}, "sha512-bbbb=="],
    };
    const result = extractPackages(packagesWithDupes, {
      allOs: true,
      noDev: false,
      devPackageNames: new Set(),
    });
    expect(result).toHaveLength(2);
    expect(result.filter((p) => p.version === "2.6.9")).toHaveLength(1);
  });

  test("skips git dependencies (entry[1] is an object)", () => {
    const packagesWithGit: Record<string, any[]> = {
      "is-number": ["is-number@7.0.0", "", {}, "sha512-abc123=="],
      "color-convert": [
        "color-convert@github:Qix-/color-convert#6d2b8b4",
        { "dependencies": {} },
        "Qix--color-convert-6d2b8b4",
      ],
    };
    const result = extractPackages(packagesWithGit, {
      allOs: true,
      noDev: false,
      devPackageNames: new Set(),
    });
    expect(result).toHaveLength(1);
    expect(result[0].name).toBe("is-number");
  });
});

describe("parseGitIdentifier", () => {
  test("parses standard github dependency", () => {
    expect(
      parseGitIdentifier("electron@github:castlabs/electron-releases#df5ab90")
    ).toEqual({
      owner: "castlabs",
      repo: "electron-releases",
      commit: "df5ab90",
    });
  });

  test("parses github dependency with hyphenated owner", () => {
    expect(
      parseGitIdentifier("color-convert@github:Qix-/color-convert#6d2b8b4")
    ).toEqual({
      owner: "Qix-",
      repo: "color-convert",
      commit: "6d2b8b4",
    });
  });

  test("parses github dependency with full commit hash", () => {
    expect(
      parseGitIdentifier(
        "node-gyp@github:electron/node-gyp#06b29aabcef8cdba9e4448c1e0e0e540fbdd66d4"
      )
    ).toEqual({
      owner: "electron",
      repo: "node-gyp",
      commit: "06b29aabcef8cdba9e4448c1e0e0e540fbdd66d4",
    });
  });

  test("returns null for non-github identifier", () => {
    expect(parseGitIdentifier("lodash@4.17.23")).toBeNull();
  });

  test("returns null for empty string", () => {
    expect(parseGitIdentifier("")).toBeNull();
  });
});

describe("extractGitPackages", () => {
  test("extracts git dependencies from packages map (3-element entries)", () => {
    const packagesMap: Record<string, any[]> = {
      "is-number": ["is-number@7.0.0", "", {}, "sha512-abc123=="],
      "electron": [
        "electron@github:castlabs/electron-releases#df5ab90",
        { "dependencies": { "@electron/get": "^2.0.0" }, "bin": { "electron": "cli.js" } },
        "castlabs-electron-releases-df5ab90",
      ],
      "@electron/node-gyp": [
        "@electron/node-gyp@github:electron/node-gyp#06b29aa",
        { "dependencies": { "env-paths": "^2.2.0" }, "bin": "./bin/node-gyp.js" },
        "electron-node-gyp-06b29aa",
      ],
    };
    const result = extractGitPackages(packagesMap);
    expect(result).toHaveLength(2);
    expect(result[0]).toEqual({
      identifier: "electron@github:castlabs/electron-releases#df5ab90",
      owner: "castlabs",
      repo: "electron-releases",
      commit: "df5ab90",
    });
    expect(result[1]).toEqual({
      identifier: "@electron/node-gyp@github:electron/node-gyp#06b29aa",
      owner: "electron",
      repo: "node-gyp",
      commit: "06b29aa",
    });
  });

  test("returns empty array when no git dependencies exist", () => {
    const packagesMap: Record<string, any[]> = {
      lodash: ["lodash@4.17.23", "", {}, "sha512-abc123=="],
    };
    const result = extractGitPackages(packagesMap);
    expect(result).toHaveLength(0);
  });

  test("deduplicates git dependencies", () => {
    const packagesMap: Record<string, any[]> = {
      "electron": [
        "electron@github:castlabs/electron-releases#df5ab90",
        { "dependencies": { "@electron/get": "^2.0.0" } },
        "castlabs-electron-releases-df5ab90",
      ],
      "some-parent/electron": [
        "electron@github:castlabs/electron-releases#df5ab90",
        { "dependencies": { "@electron/get": "^2.0.0" } },
        "castlabs-electron-releases-df5ab90",
      ],
    };
    const result = extractGitPackages(packagesMap);
    expect(result).toHaveLength(1);
  });
});

describe("gitPkgToFlatpakSource", () => {
  test("generates archive source with correct cache path", () => {
    const pkg = {
      identifier: "electron@github:castlabs/electron-releases#df5ab90",
      owner: "castlabs",
      repo: "electron-releases",
      commit: "df5ab90",
    };
    const source = gitPkgToFlatpakSource(pkg, "abcdef1234567890");
    expect(source).toEqual({
      type: "archive",
      url: "https://github.com/castlabs/electron-releases/archive/df5ab90.tar.gz",
      sha256: "abcdef1234567890",
      dest: "bun_cache/@GH@castlabs-electron-releases-df5ab90@@@1",
      "strip-components": 1,
    });
  });

  test("generates correct path for different owner/repo", () => {
    const pkg = {
      identifier: "node-gyp@github:electron/node-gyp#06b29aa",
      owner: "electron",
      repo: "node-gyp",
      commit: "06b29aa",
    };
    const source = gitPkgToFlatpakSource(pkg, "fedcba0987654321");
    expect(source.url).toBe(
      "https://github.com/electron/node-gyp/archive/06b29aa.tar.gz"
    );
    expect(source.dest).toBe(
      "bun_cache/@GH@electron-node-gyp-06b29aa@@@1"
    );
    expect(source["strip-components"]).toBe(1);
  });
});

describe("npmPkgToFlatpakSources", () => {
  test("generates file source with correct bun cache dest-filename", () => {
    const pkg = {
      identifier: "lodash@4.17.23",
      name: "lodash",
      version: "4.17.23",
      integrity: "sha512-abc123==",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(1);

    const src = sources[0];
    expect(src.type).toBe("file");
    expect(src.url).toBe(
      "https://registry.npmjs.org/lodash/-/lodash-4.17.23.tgz"
    );
    expect(src.dest).toBe("bun_cache");
    expect(src["dest-filename"]).toBe("lodash@4.17.23.tgz");
    expect(src.sha512).toBeDefined();
  });

  test("handles scoped packages correctly", () => {
    const pkg = {
      identifier: "@babel/core@7.29.0",
      name: "@babel/core",
      version: "7.29.0",
      integrity: "sha512-def456==",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(1);

    const src = sources[0];
    expect(src.url).toBe(
      "https://registry.npmjs.org/@babel/core/-/core-7.29.0.tgz"
    );
    expect(src.dest).toBe("bun_cache");
    expect(src["dest-filename"]).toBe("@babel--core@7.29.0.tgz");
  });

  test("adds only-arches for CPU-specific packages", () => {
    const pkg = {
      identifier: "@rollup/rollup-linux-x64-gnu@4.59.0",
      name: "@rollup/rollup-linux-x64-gnu",
      version: "4.59.0",
      integrity: "sha512-aGVsbG8=",
      cpu: "x64",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(1);
    expect(sources[0]["only-arches"]).toEqual(["x86_64"]);
  });

  test("returns empty array for unparseable integrity hash", () => {
    const pkg = {
      identifier: "bad@1.0.0",
      name: "bad",
      version: "1.0.0",
      integrity: "nohash",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(0);
  });

  test("hashes pre-release version in dest-filename", () => {
    const pkg = {
      identifier: "gensync@1.0.0-beta.2",
      name: "gensync",
      version: "1.0.0-beta.2",
      integrity: "sha512-abc123==",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(1);

    const src = sources[0];
    expect(src.url).toBe(
      "https://registry.npmjs.org/gensync/-/gensync-1.0.0-beta.2.tgz"
    );
    expect(src["dest-filename"]).toBe("gensync@1.0.0-4049f5e8f1219d89.tgz");
  });

  test("hashes pre-release version for scoped packages", () => {
    const pkg = {
      identifier: "@rolldown/pluginutils@1.0.0-rc.3",
      name: "@rolldown/pluginutils",
      version: "1.0.0-rc.3",
      integrity: "sha512-def456==",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(1);

    const src = sources[0];
    expect(src.url).toBe(
      "https://registry.npmjs.org/@rolldown/pluginutils/-/pluginutils-1.0.0-rc.3.tgz"
    );
    expect(src["dest-filename"]).toBe("@rolldown--pluginutils@1.0.0-740e203097086e5e.tgz");
  });

  test("does not hash regular versions without pre-release", () => {
    const pkg = {
      identifier: "lodash@4.17.23",
      name: "lodash",
      version: "4.17.23",
      integrity: "sha512-abc123==",
    };

    const sources = npmPkgToFlatpakSources(
      pkg,
      "https://registry.npmjs.org"
    );

    expect(sources).toHaveLength(1);
    expect(sources[0]["dest-filename"]).toBe("lodash@4.17.23.tgz");
  });
});

describe("collectDevDependencyNames", () => {
  test("identifies dev-only packages", () => {
    const workspaces = {
      "": {
        name: "test",
        dependencies: { lodash: "^4.17.21" },
        devDependencies: { typescript: "^5.0.0" },
      },
    };

    const packages: Record<string, any[]> = {
      lodash: ["lodash@4.17.23", "", {}, "sha512-a=="],
      typescript: ["typescript@5.9.3", "", {}, "sha512-b=="],
    };

    const devOnly = collectDevDependencyNames(workspaces, packages);
    expect(devOnly.has("typescript")).toBe(true);
    expect(devOnly.has("lodash")).toBe(false);
  });

  test("includes transitive deps of prod packages as prod", () => {
    const workspaces = {
      "": {
        name: "test",
        dependencies: { express: "^4.18.0" },
        devDependencies: { typescript: "^5.0.0" },
      },
    };

    const packages: Record<string, any[]> = {
      express: [
        "express@4.22.1",
        "",
        { dependencies: { accepts: "~1.3.8" } },
        "sha512-a==",
      ],
      accepts: ["accepts@1.3.8", "", {}, "sha512-b=="],
      typescript: ["typescript@5.9.3", "", {}, "sha512-c=="],
    };

    const devOnly = collectDevDependencyNames(workspaces, packages);
    expect(devOnly.has("typescript")).toBe(true);
    expect(devOnly.has("express")).toBe(false);
    expect(devOnly.has("accepts")).toBe(false);
  });

  test("resolves nested dependency paths as prod-reachable", () => {
    const workspaces = {
      "": {
        name: "test",
        dependencies: { express: "^4.18.0" },
        devDependencies: { typescript: "^5.0.0" },
      },
    };

    const packages: Record<string, any[]> = {
      express: [
        "express@4.22.1",
        "",
        { dependencies: { debug: "2.6.9" } },
        "sha512-a==",
      ],
      debug: ["debug@4.4.3", "", {}, "sha512-b=="],
      "express/debug": [
        "debug@2.6.9",
        "",
        { dependencies: { ms: "2.0.0" } },
        "sha512-c==",
      ],
      "express/debug/ms": ["ms@2.0.0", "", {}, "sha512-d=="],
      ms: ["ms@2.1.3", "", {}, "sha512-e=="],
      typescript: ["typescript@5.9.3", "", {}, "sha512-f=="],
    };

    const devOnly = collectDevDependencyNames(workspaces, packages);
    expect(devOnly.has("express")).toBe(false);
    expect(devOnly.has("express/debug")).toBe(false);
    expect(devOnly.has("express/debug/ms")).toBe(false);
    expect(devOnly.has("typescript")).toBe(true);
    expect(devOnly.has("debug")).toBe(true);
  });
});
