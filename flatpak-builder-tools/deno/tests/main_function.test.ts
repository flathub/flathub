// LICENSE = MIT
// deno-lint-ignore-file no-explicit-any no-import-prefix
import { main } from "../src/main.ts";
import { assert } from "jsr:@std/assert@1.0.16";
import { join } from "jsr:@std/path@1.1.4";
import { existsSync } from "jsr:@std/fs@1.0.21";

Deno.test("main function: generates deno-sources.json from lockfile", async () => {
  const tmpDir = "./tests/tmp_main";
  await Deno.mkdir(tmpDir, { recursive: true });
  const lockPath = join(tmpDir, "deno.lock");
  const sourcesPath = join(tmpDir, "deno-sources.json");

  // Lockfile with jsr, npm, and https deps
  await Deno.writeTextFile(
    lockPath,
    JSON.stringify(
      {
        version: "5",
        jsr: {
          "@std/encoding@1.0.10": {
            integrity:
              "8783c6384a2d13abd5e9e87a7ae0520a30e9f56aeeaa3bdf910a3eaaf5c811a1",
          },
        },
        npm: {
          "left-pad@1.3.0": {
            integrity:
              "sha512-1r9Z1tcHTul3e8DqRLVQjaxAg/P6nxsVXni4eWh05rq6ArlTc95xJMu38xpv8uKXuX4nHCqB6f+GO6zkRgLr1w==",
            engines: { node: ">=0.10.0" },
          },
          // peer dep
          "update-browserslist-db@1.1.3_browserslist@4.24.4": {
            "integrity":
              "sha512-UxhIZQ+QInVdunkDAaiazvvT/+fXL5Osr0JZlJulepYu6Jd7qJtDZjlur0emRlT71EN3ScPoE7gvsuIKKNavKw==",
            "dependencies": [
              "browserslist",
              "escalade",
              "picocolors",
            ],
            "bin": true,
          },
          // peer deps can have multiple peers
          "@sveltejs/vite-plugin-svelte-inspector@4.0.1_@sveltejs+vite-plugin-svelte@5.0.3__svelte@5.25.3___acorn@8.14.1__vite@6.2.3_svelte@5.25.3__acorn@8.14.1_vite@6.2.3":
            {
              "integrity":
                "sha512-J/Nmb2Q2y7mck2hyCX4ckVHcR5tu2J+MtBEQqpDrrgELZ2uvraQcK/ioCV61AqkdXFgriksOKIceDcQmqnGhVw==",
              "dependencies": [
                "@sveltejs/vite-plugin-svelte",
                "debug",
                "svelte",
                "vite",
              ],
            },
        },
        remote: {
          "https://deno.land/std@0.203.0/uuid/v1.ts":
            "b6e2e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1e2c1",
        },
      },
      null,
      2,
    ),
  );

  try {
    await main(lockPath, sourcesPath);
    assert(existsSync(sourcesPath), "deno-sources.json should be created");
    const sources = JSON.parse(await Deno.readTextFile(sourcesPath));
    assert(Array.isArray(sources));
    // jsr checks
    assert(sources.some((s: any) => s["dest-filename"] === "meta.json"));
    assert(sources.some((s: any) => s["dest-filename"] === "1.0.10_meta.json"));
    // npm checks
    assert(sources.some((s: any) => s["dest-filename"] === "registry.json"));
    assert(sources.some((s: any) =>
      typeof s.dest === "string" &&
      s.dest.includes("left-pad/1.3.0")
    ));
    // https checks
    assert(sources.some((s: any) =>
      typeof s.url === "string" &&
      s.url.startsWith("https://deno.land/std@0.203.0/uuid/v1.ts")
    ));
  } finally {
    await Deno.remove(tmpDir, { recursive: true });
  }
});

Deno.test("main function: handles JSR packages via npm.jsr.io", async () => {
  const tmpDir = "./tests/tmp_jsr_npm";
  await Deno.mkdir(tmpDir, { recursive: true });
  const lockPath = join(tmpDir, "deno.lock");
  const sourcesPath = join(tmpDir, "deno-sources.json");

  // Create a lock file with JSR packages accessed via npm.jsr.io
  await Deno.writeTextFile(
    lockPath,
    JSON.stringify(
      {
        version: "5",
        npm: {
          "@jsr/sigma__deno-compat@0.9.0": {
            "integrity":
              "sha512-aR+PgQ2FXHc94QKFJTKpSl7W1PlL8iECN7wMcNbjVCOtPjqgcYS8qEKbiN7W1d/TvxgwFJmSSIpWXwtXw+NDmg==",
            "tarball":
              "https://npm.jsr.io/~/11/@jsr/sigma__deno-compat/0.9.0.tgz",
          },
        },
      },
      null,
      2,
    ),
  );

  try {
    await main(lockPath, sourcesPath);
    assert(existsSync(sourcesPath), "deno-sources.json should be created");
    const sources = JSON.parse(await Deno.readTextFile(sourcesPath));
    assert(Array.isArray(sources));

    // Check that JSR packages are handled correctly
    // Should have entries for @jsr/sigma__deno-compat
    const jsrNpmEntries = sources.filter((s: any) =>
      s.dest && s.dest.startsWith("deno_dir/npm/npm.jsr.io")
    );
    assert(
      jsrNpmEntries.length > 0,
      "Should have entries for JSR packages accessed via npm.jsr.io",
    );

    // Verify the path structure is correct: npm/npm.jsr.io/@jsr/sigma__deno-compat/0.9.0/
    const denoCompatEntry = sources.find((s: any) =>
      s.dest && s.dest.includes("@jsr/sigma__deno-compat/0.9.0")
    );
    assert(
      denoCompatEntry,
      "Should have entry for @jsr/sigma__deno-compat@0.9.0",
    );
    assert(
      denoCompatEntry.dest ===
        "deno_dir/npm/npm.jsr.io/@jsr/sigma__deno-compat/0.9.0",
      `Expected path deno_dir/npm/npm.jsr.io/@jsr/sigma__deno-compat/0.9.0, got ${denoCompatEntry.dest}`,
    );

    // Verify tarball URL points to npm.jsr.io
    assert(
      denoCompatEntry.url &&
        new URL(denoCompatEntry.url).hostname === "npm.jsr.io",
      "Tarball URL should point to npm.jsr.io",
    );
  } finally {
    await Deno.remove(tmpDir, { recursive: true });
  }
});
