import { describe, expect, test, afterAll } from "bun:test";
import { main } from "../src/main.ts";
import { existsSync, mkdirSync, writeFileSync, readFileSync, rmSync } from "fs";
import { join } from "path";

const TMP_DIR = join(import.meta.dir, "tmp_main_function");

const HASH_A = "sW7X0ks+y9QWTc2tN04IwKt1GKoH+dNoPzTCs8Z6FYMCaMtKVsH/b1TI5Up5X1uHwIZotR+C0Ak/e67n0pgRgQ==";
const HASH_B = "bSAb7u+1ibCO8GctrII1PQy9mtmeFkLIOhYB89ZHvMoAMle16PMb3B1z++yE+whcedbiZ3t/+SfoI6VOeJFA2Q==";
const HASH_C = "y4ct4rjSUJxUNEQ1zpy0O0+qJ/l9SG/03jWvA+SRn7TsUyZ8r43vBu8XfWn+CrqzwS+9wvJn2JX9B8NqYr/0vw==";
const HASH_D = "IleqtEtCgTFCqorEdnEWrVvUHpSnmqBnLMliEo7UgJ9Q7TjTW6lFqAeZl2ye+ptobyjRgDYTS8K7CsLeluxigA==";
const HASH_E = "41Cifkg6e8TylSpdtTpeLVMqvSBEVzTttHvERD741+pnZ8ANv0004MRL43QKPDlK9cGvNp6NZWZUBlbGXYxxng==";

afterAll(() => {
  if (existsSync(TMP_DIR)) {
    rmSync(TMP_DIR, { recursive: true });
  }
});

describe("main function integration", () => {
  test("generates sources JSON from a simple lockfile", async () => {
    const testDir = join(TMP_DIR, "simple");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              "is-number": "7.0.0",
            },
          },
        },
        packages: {
          "is-number": [
            "is-number@7.0.0",
            "",
            {},
            "sha512-41Cifkg6e8TylSpdtTpeLVMqvSBEVzTttHvERD741+pnZ8ANv0004MRL43QKPDlK9cGvNp6NZWZUBlbGXYxxng==",
          ],
        },
      })
    );

    await main(lockPath, outputPath);

    expect(existsSync(outputPath)).toBe(true);

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));
    expect(Array.isArray(sources)).toBe(true);
    expect(sources).toHaveLength(1);

    const src = sources[0];
    expect(src.type).toBe("file");
    expect(src.dest).toBe("bun_cache");
    expect(src["dest-filename"]).toBe("is-number@7.0.0.tgz");
    expect(src.url).toBe(
      "https://registry.npmjs.org/is-number/-/is-number-7.0.0.tgz"
    );
    expect(src.sha512).toBeDefined();
  });

  test("generates sources for scoped packages", async () => {
    const testDir = join(TMP_DIR, "scoped");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              "@types/node": "22.13.9",
            },
          },
        },
        packages: {
          "@types/node": [
            "@types/node@22.13.9",
            "",
            { dependencies: { "undici-types": "~6.20.0" } },
            `sha512-${HASH_A}`,
          ],
          "undici-types": [
            "undici-types@6.20.0",
            "",
            {},
            `sha512-${HASH_B}`,
          ],
        },
      })
    );

    await main(lockPath, outputPath);

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));
    expect(sources).toHaveLength(2);

    const typesNode = sources.find(
      (s: any) => s["dest-filename"] === "@types--node@22.13.9.tgz"
    );
    expect(typesNode).toBeDefined();
    expect(typesNode.url).toBe(
      "https://registry.npmjs.org/@types/node/-/node-22.13.9.tgz"
    );

    const undici = sources.find(
      (s: any) => s["dest-filename"] === "undici-types@6.20.0.tgz"
    );
    expect(undici).toBeDefined();
  });

  test("filters dev dependencies with --no-devel", async () => {
    const testDir = join(TMP_DIR, "nodev");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              "is-number": "7.0.0",
            },
            devDependencies: {
              typescript: "^5.0.0",
            },
          },
        },
        packages: {
          "is-number": [
            "is-number@7.0.0",
            "",
            {},
            "sha512-41Cifkg6e8TylSpdtTpeLVMqvSBEVzTttHvERD741+pnZ8ANv0004MRL43QKPDlK9cGvNp6NZWZUBlbGXYxxng==",
          ],
          typescript: [
            "typescript@5.9.3",
            "",
            {},
            `sha512-${HASH_A}`,
          ],
        },
      })
    );

    await main(lockPath, outputPath, { noDev: true });

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));
    expect(sources).toHaveLength(1);
    expect(sources[0].dest).toBe("bun_cache");
    expect(sources[0]["dest-filename"]).toBe("is-number@7.0.0.tgz");
  });

  test("includes platform-specific packages with only-arches", async () => {
    const testDir = join(TMP_DIR, "platform");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              "@rollup/rollup-linux-x64-gnu": "4.35.0",
            },
          },
        },
        packages: {
          "@rollup/rollup-linux-x64-gnu": [
            "@rollup/rollup-linux-x64-gnu@4.35.0",
            "",
            { os: "linux", cpu: "x64" },
            `sha512-${HASH_A}`,
          ],
        },
      })
    );

    await main(lockPath, outputPath);

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));
    expect(sources).toHaveLength(1);
    expect(sources[0]["only-arches"]).toEqual(["x86_64"]);
    expect(sources[0].dest).toBe("bun_cache");
    expect(sources[0]["dest-filename"]).toBe(
      "@rollup--rollup-linux-x64-gnu@4.35.0.tgz"
    );
  });

  test("uses custom registry URL", async () => {
    const testDir = join(TMP_DIR, "registry");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: { "is-number": "7.0.0" },
          },
        },
        packages: {
          "is-number": [
            "is-number@7.0.0",
            "",
            {},
            `sha512-${HASH_E}`,
          ],
        },
      })
    );

    await main(lockPath, outputPath, {
      registry: "https://my-registry.example.com/",
    });

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));
    expect(sources[0].url).toStartWith(
      "https://my-registry.example.com/is-number/-/"
    );
  });

  test("deduplicates nested dependency entries", async () => {
    const testDir = join(TMP_DIR, "dedup");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              express: "^4.18.0",
              debug: "^4.0.0",
            },
          },
        },
        packages: {
          debug: ["debug@4.4.3", "", {}, `sha512-${HASH_A}`],
          "body-parser/debug": ["debug@2.6.9", "", {}, `sha512-${HASH_B}`],
          "express/debug": ["debug@2.6.9", "", {}, `sha512-${HASH_B}`],
          express: ["express@4.22.1", "", {}, `sha512-${HASH_C}`],
          "body-parser": ["body-parser@1.20.3", "", {}, `sha512-${HASH_D}`],
        },
      })
    );

    await main(lockPath, outputPath);

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));
    expect(sources).toHaveLength(4);

    const debugDests = sources
      .filter((s: any) => s["dest-filename"]?.includes("debug@"))
      .map((s: any) => s["dest-filename"]);
    expect(debugDests).toContain("debug@4.4.3.tgz");
    expect(debugDests).toContain("debug@2.6.9.tgz");
    expect(debugDests).toHaveLength(2);
  });

  test("includes git dependencies as archive sources", async () => {
    const testDir = join(TMP_DIR, "gitdeps");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              "is-number": "7.0.0",
              "node-gyp": "github:electron/node-gyp#06b29aa",
            },
          },
        },
        packages: {
          "is-number": [
            "is-number@7.0.0",
            "",
            {},
            `sha512-${HASH_E}`,
          ],
          "node-gyp": [
            "node-gyp@github:electron/node-gyp#06b29aa",
            { "dependencies": { "env-paths": "^2.2.0" }, "bin": "./bin/node-gyp.js" },
            "electron-node-gyp-06b29aa",
          ],
        },
      })
    );

    await main(lockPath, outputPath);

    expect(existsSync(outputPath)).toBe(true);

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));

    const npmSources = sources.filter((s: any) => s.type === "file");
    const gitSources = sources.filter((s: any) => s.type === "archive");

    expect(npmSources).toHaveLength(1);
    expect(npmSources[0]["dest-filename"]).toBe("is-number@7.0.0.tgz");

    expect(gitSources).toHaveLength(1);
    expect(gitSources[0].url).toBe(
      "https://github.com/electron/node-gyp/archive/06b29aa.tar.gz"
    );
    expect(gitSources[0].dest).toBe(
      "bun_cache/@GH@electron-node-gyp-06b29aa@@@1"
    );
    expect(gitSources[0]["strip-components"]).toBe(1);
    expect(gitSources[0].sha256).toBeDefined();
    expect(typeof gitSources[0].sha256).toBe("string");
    expect(gitSources[0].sha256.length).toBeGreaterThan(0);
  });

  test("generates electron binary and node headers sources for castlabs dep", async () => {
    const testDir = join(TMP_DIR, "electron");
    mkdirSync(testDir, { recursive: true });

    const lockPath = join(testDir, "bun.lock");
    const outputPath = join(testDir, "bun-sources.json");

    writeFileSync(
      lockPath,
      JSON.stringify({
        lockfileVersion: 1,
        configVersion: 1,
        workspaces: {
          "": {
            name: "test",
            dependencies: {
              "is-number": "7.0.0",
              "electron": "github:castlabs/electron-releases#df5ab90",
            },
          },
        },
        packages: {
          "is-number": [
            "is-number@7.0.0",
            "",
            {},
            `sha512-${HASH_E}`,
          ],
          "electron": [
            "electron@github:castlabs/electron-releases#df5ab90",
            { "dependencies": { "@electron/get": "^2.0.0" }, "bin": { "electron": "cli.js" } },
            "castlabs-electron-releases-df5ab90",
          ],
        },
      })
    );

    await main(lockPath, outputPath);

    expect(existsSync(outputPath)).toBe(true);

    const sources = JSON.parse(readFileSync(outputPath, "utf-8"));

    // npm source for is-number
    const npmSources = sources.filter(
      (s: any) => s.type === "file" && s.dest === "bun_cache"
    );
    expect(npmSources).toHaveLength(1);
    expect(npmSources[0]["dest-filename"]).toBe("is-number@7.0.0.tgz");

    // git archive source for castlabs/electron-releases
    const gitSources = sources.filter(
      (s: any) => s.type === "archive" && s.dest?.startsWith("bun_cache/")
    );
    expect(gitSources).toHaveLength(1);
    expect(gitSources[0].url).toBe(
      "https://github.com/castlabs/electron-releases/archive/df5ab90.tar.gz"
    );

    // electron binary zip sources (at least x64, arm64 may not exist for all releases)
    const electronBinSources = sources.filter(
      (s: any) => s.type === "file" && s.dest?.startsWith("electron-cache/")
    );
    expect(electronBinSources.length).toBeGreaterThanOrEqual(1);

    const x64Bin = electronBinSources.find(
      (s: any) => s["only-arches"]?.[0] === "x86_64"
    );
    expect(x64Bin).toBeDefined();
    expect(x64Bin.url).toBe(
      "https://github.com/castlabs/electron-releases/releases/download/v40.1.0%2Bwvcus/electron-v40.1.0+wvcus-linux-x64.zip"
    );
    expect(x64Bin["dest-filename"]).toBe("electron-v40.1.0+wvcus-linux-x64.zip");
    expect(x64Bin.dest).toBe(
      "electron-cache/ba836dbb76e179a4c41de2ac3b52efdaff73f6355c1d7b224e1d9e4251ed220c"
    );
    expect(x64Bin.sha256).toBeDefined();
    expect(x64Bin.sha256.length).toBe(64);

    // arm64 may or may not be available depending on the release
    const arm64Bin = electronBinSources.find(
      (s: any) => s["only-arches"]?.[0] === "aarch64"
    );
    if (arm64Bin) {
      expect(arm64Bin.url).toContain("linux-arm64.zip");
      expect(arm64Bin["dest-filename"]).toBe("electron-v40.1.0+wvcus-linux-arm64.zip");
    }

    // node headers source
    const headersSources = sources.filter(
      (s: any) => s.type === "archive" && s.dest === "electron-headers"
    );
    expect(headersSources).toHaveLength(1);
    expect(headersSources[0].url).toBe(
      "https://artifacts.electronjs.org/headers/dist/v40.1.0/node-v40.1.0-headers.tar.gz"
    );
    expect(headersSources[0]["strip-components"]).toBe(1);
    expect(headersSources[0].sha256).toBeDefined();
    expect(headersSources[0].sha256.length).toBe(64);
  }, 300_000); // 5 minute timeout for downloading electron binaries
});
