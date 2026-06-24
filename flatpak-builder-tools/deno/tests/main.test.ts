// LICENSE = MIT
// deno-lint-ignore-file require-await
// deno-lint-ignore no-import-prefix
import { assert, assertEquals, assertMatch } from "jsr:@std/assert@1.0.16";
import {
  jsrNpmPkgToFlatpakData,
  jsrPkgToFlatpakData,
  npmPkgToFlatpakData,
} from "../src/main.ts";

Deno.test("jsrPkgToFlatpakData returns correct flatpak data", async () => {
  // Mock fetch for meta.json and versioned meta
  const metaJson = {
    scope: "@std",
    name: "encoding",
    latest: "1.0.10",
  };
  const metaVerJson = JSON.stringify({
    moduleGraph2: {
      "/mod.ts": {},
      "/deno.json": {},
    },
    moduleGraph1: {},
    manifest: {
      "/mod.ts": {
        checksum: "sha256-abcdef1234567890",
      },
      "/deno.json": {
        checksum: "sha256-ffeeddccbbaa9988",
      },
    },
  });

  let fetchCallCount = 0;
  const origFetch = globalThis.fetch;
  Object.defineProperty(globalThis, "fetch", {
    configurable: true,
    writable: true,
    value: async (input: URL | RequestInfo, _init?: RequestInit) => {
      const url = typeof input === "string"
        ? input
        : input instanceof URL
        ? input.toString()
        : (input as Request).url;
      fetchCallCount++;
      if (url.endsWith("_meta.json")) {
        return {
          text: async () => metaVerJson,
        } as Response;
      }
      if (url.endsWith("meta.json")) {
        return {
          json: async () => metaJson,
        } as Response;
      }
      throw new Error("Unexpected fetch url: " + url);
    },
  });

  const pkg = {
    module: "@std/encoding",
    version: "1.0.10",
    name: "encoding",
  };

  const data = await jsrPkgToFlatpakData(pkg);
  // Restore fetch after test
  Object.defineProperty(globalThis, "fetch", {
    configurable: true,
    writable: true,
    value: origFetch,
  });

  // Should have meta.json, versioned meta, /mod.ts, deno.json, and duplicate deno.json
  assertEquals(data.length, 5);

  // meta.json should be inline type with specific contents
  assertEquals(data[0].type, "inline");
  assertEquals(data[0]["dest-filename"], "meta.json");
  const inlineContents = JSON.parse(data[0].contents as string);
  assertEquals(inlineContents.scope, "@std");
  assertEquals(inlineContents.name, "encoding");
  assertEquals(inlineContents.latest, "1.0.10");
  assertEquals(inlineContents.versions, {});
  assertEquals(data[1].url, "https://jsr.io/@std/encoding/1.0.10_meta.json");
  assertEquals(data[1]["dest-filename"], "1.0.10_meta.json");

  // /mod.ts
  assertEquals(data[2].url, "https://jsr.io/@std/encoding/1.0.10/mod.ts");
  assertEquals(data[2].sha256, "abcdef1234567890");
  // Accept either "mod.ts" or a hashed filename
  assertMatch(
    data[2]["dest-filename"] as string,
    /^mod\.ts$|^#mod_[a-f0-9]{5}\.ts$/,
  );

  // /deno.json
  assertEquals(data[3].url, "https://jsr.io/@std/encoding/1.0.10/deno.json");
  assertEquals(data[3].sha256, "ffeeddccbbaa9988");
  assertEquals(data[3]["dest-filename"], "deno.json");
});

Deno.test("jsrPkgToFlatpakData hashes directory segments", async () => {
  const metaJson = {
    scope: "@sigmasd",
    name: "gtk",
    latest: "0.13.1",
  };
  const metaVerJson = JSON.stringify({
    moduleGraph2: {
      "/src/libPaths/mod.ts": {},
    },
    manifest: {
      "/src/libPaths/mod.ts": {
        checksum: "sha256-abcdef",
      },
    },
  });

  const origFetch = globalThis.fetch;
  Object.defineProperty(globalThis, "fetch", {
    configurable: true,
    writable: true,
    value: async (input: URL | RequestInfo, _init?: RequestInit) => {
      const url = typeof input === "string"
        ? input
        : input instanceof URL
        ? input.toString()
        : (input as Request).url;
      if (url.endsWith("_meta.json")) {
        return { text: async () => metaVerJson } as Response;
      }
      if (url.endsWith("meta.json")) {
        return { json: async () => metaJson } as Response;
      }
      throw new Error("Unexpected fetch url: " + url);
    },
  });

  try {
    const pkg = { module: "@sigmasd/gtk", version: "0.13.1", name: "gtk" };
    const data = await jsrPkgToFlatpakData(pkg);

    const modFile = data.find((d) =>
      d.url === "https://jsr.io/@sigmasd/gtk/0.13.1/src/libPaths/mod.ts"
    );
    assert(modFile, "Should find mod.ts");

    // "libPaths" should be hashed to "#libpaths_8b87f"
    assert(
      modFile.dest.includes("#libpaths_8b87f"),
      `Path should be hashed, got: ${modFile.dest}`,
    );
    assertEquals(modFile["dest-filename"], "mod.ts");
  } finally {
    Object.defineProperty(globalThis, "fetch", {
      configurable: true,
      writable: true,
      value: origFetch,
    });
  }
});

Deno.test("npmPkgToFlatpakData returns correct flatpak data", async () => {
  // Mock fetch for npm meta
  const metaJson = {
    versions: {
      "2.18.4": {
        dist: {
          // "abcdefg" in base64 is "YWJjZGVmZw=="
          integrity: "sha512-YWJjZGVmZw==",
        },
      },
    },
  };

  const origFetch = globalThis.fetch;
  Object.defineProperty(globalThis, "fetch", {
    configurable: true,
    writable: true,
    value: async (input: URL | RequestInfo, _init?: RequestInit) => {
      const url = typeof input === "string"
        ? input
        : input instanceof URL
        ? input.toString()
        : (input as Request).url;
      if (url === "https://registry.npmjs.org/@napi-rs/cli") {
        return {
          json: async () => metaJson,
        } as Response;
      }
      throw new Error("Unexpected fetch url: " + url);
    },
  });

  const pkg = {
    module: "@napi-rs/cli",
    version: "2.18.4",
    name: "cli",
    cpu: "x86_64" as const,
  };

  const data = await npmPkgToFlatpakData(pkg);
  // Restore fetch after test
  Object.defineProperty(globalThis, "fetch", {
    configurable: true,
    writable: true,
    value: origFetch,
  });

  // Should have registry.json and archive
  assertEquals(data.length, 2);

  // registry.json
  const registryContents = data.at(0)?.contents;
  assert(registryContents !== undefined);
  assert("2.18.4" in JSON.parse(registryContents).versions);
  assertEquals(data[0]["dest-filename"], "registry.json");

  // archive
  assertEquals(
    data[1].url,
    "https://registry.npmjs.org/@napi-rs/cli/-/cli-2.18.4.tgz",
  );
  assertEquals(
    data[1]["archive-type"],
    "tar-gzip",
  );
  assertEquals(
    data[1].dest,
    "deno_dir/npm/registry.npmjs.org/@napi-rs/cli/2.18.4",
  );
  assertEquals(
    (data[1]["only-arches"] as string[])[0],
    "x86_64",
  );
  // sha512 should be present and hex
  assertMatch(
    String(data[1].sha512),
    /^[a-f0-9]+$/,
  );
});

Deno.test("jsrNpmPkgToFlatpakData handles JSR packages via npm.jsr.io", async () => {
  // JSR packages accessed via npm compatibility layer have:
  // - Package names like "@jsr/scope__package"
  // - Tarball URLs pointing to npm.jsr.io
  // - Direct integrity hash in lock file (no registry fetch needed)

  // This test simulates what we get from the lock file for:
  // "@jsr/sigma__deno-compat@0.9.0": {
  //   "integrity": "sha512-aR+PgQ2FXHc94QKFJTKpSl7W1PlL8iECN7wMcNbjVCOtPjqgcYS8qEKbiN7W1d/TvxgwFJmSSIpWXwtXw+NDmg==",
  //   "tarball": "https://npm.jsr.io/~/11/@jsr/sigma__deno-compat/0.9.0.tgz"
  // }

  const pkg = {
    module: "@jsr/sigma__deno-compat",
    version: "0.9.0",
    name: "sigma__deno-compat",
  };

  const lockData = {
    integrity:
      "sha512-aR+PgQ2FXHc94QKFJTKpSl7W1PlL8iECN7wMcNbjVCOtPjqgcYS8qEKbiN7W1d/TvxgwFJmSSIpWXwtXw+NDmg==",
    tarball: "https://npm.jsr.io/~/11/@jsr/sigma__deno-compat/0.9.0.tgz",
  };

  // Should not need any fetch calls - data comes from lock file
  const origFetch = globalThis.fetch;
  let fetchCalled = false;
  Object.defineProperty(globalThis, "fetch", {
    configurable: true,
    writable: true,
    value: async (_input: URL | RequestInfo, _init?: RequestInit) => {
      fetchCalled = true;
      throw new Error(
        "Should not fetch for JSR packages - data is in lock file",
      );
    },
  });

  try {
    const data = await jsrNpmPkgToFlatpakData(pkg, lockData);

    // Should have 4 entries: 2 registry.json (npm.jsr.io + registry.npmjs.org) + 2 archives
    assertEquals(data.length, 4);

    // Should not have called fetch (data comes from lock file)
    assertEquals(fetchCalled, false, "Should not fetch for JSR packages");

    // registry.json for npm.jsr.io path
    assertEquals(data[0].type, "inline");
    assertEquals(data[0]["dest-filename"], "registry.json");
    assertEquals(
      data[0].dest,
      "deno_dir/npm/npm.jsr.io/@jsr/sigma__deno-compat",
    );
    const registryContents1 = JSON.parse(data[0].contents as string);
    assert(registryContents1.versions["0.9.0"]);

    // registry.json for registry.npmjs.org path
    assertEquals(data[1].type, "inline");
    assertEquals(data[1]["dest-filename"], "registry.json");
    assertEquals(
      data[1].dest,
      "deno_dir/npm/registry.npmjs.org/@jsr/sigma__deno-compat",
    );
    const registryContents2 = JSON.parse(data[1].contents as string);
    assert(registryContents2.versions["0.9.0"]);

    // Archive for npm.jsr.io path
    assertEquals(data[2].type, "archive");
    assertEquals(data[2]["archive-type"], "tar-gzip");
    assertMatch(
      data[2].url as string,
      /npm\.jsr\.io.*@jsr.*sigma__deno-compat.*0\.9\.0\.tgz/,
    );
    assertEquals(
      data[2].dest,
      "deno_dir/npm/npm.jsr.io/@jsr/sigma__deno-compat/0.9.0",
    );
    assertMatch(
      String(data[2].sha512),
      /^[a-f0-9]+$/,
    );

    // Archive for registry.npmjs.org path
    assertEquals(data[3].type, "archive");
    assertEquals(data[3]["archive-type"], "tar-gzip");
    assertMatch(
      data[3].url as string,
      /npm\.jsr\.io.*@jsr.*sigma__deno-compat.*0\.9\.0\.tgz/,
    );
    assertEquals(
      data[3].dest,
      "deno_dir/npm/registry.npmjs.org/@jsr/sigma__deno-compat/0.9.0",
    );
    assertMatch(
      String(data[3].sha512),
      /^[a-f0-9]+$/,
    );
  } finally {
    // Restore fetch after test
    Object.defineProperty(globalThis, "fetch", {
      configurable: true,
      writable: true,
      value: origFetch,
    });
  }
});
