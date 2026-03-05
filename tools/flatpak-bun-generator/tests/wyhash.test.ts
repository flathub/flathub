import { describe, expect, test } from "bun:test";
import { wyhash, wyhashHex, extractPreRelease, bunCacheVersion } from "../src/wyhash.ts";

describe("wyhash", () => {
  test("hashes empty string", () => {
    const result = wyhash(0n, new Uint8Array(0));
    expect(typeof result).toBe("bigint");
  });

  test("returns different hashes for different inputs", () => {
    const enc = new TextEncoder();
    const h1 = wyhash(0n, enc.encode("alpha"));
    const h2 = wyhash(0n, enc.encode("beta"));
    expect(h1).not.toBe(h2);
  });

  test("returns different hashes for different seeds", () => {
    const enc = new TextEncoder();
    const h1 = wyhash(0n, enc.encode("test"));
    const h2 = wyhash(42n, enc.encode("test"));
    expect(h1).not.toBe(h2);
  });
});

describe("wyhashHex", () => {
  test("returns 16-character lowercase hex string", () => {
    const result = wyhashHex("test");
    expect(result).toHaveLength(16);
    expect(result).toMatch(/^[0-9a-f]{16}$/);
  });

  test("matches Bun's known pre-release tag hashes", () => {
    const knownHashes: [string, string][] = [
      ["beta.2", "4049f5e8f1219d89"],
      ["next.5", "5f76f0da6d05cfd7"],
      ["alpha.6", "32bd5ff2c1a25ef6"],
      ["alpha.12", "93f69ee49b615ee6"],
      ["rc1", "0eeaba7239b72d52"],
    ];

    for (const [input, expected] of knownHashes) {
      expect(wyhashHex(input)).toBe(expected);
    }
  });

  test("pads short hashes with leading zeros", () => {
    const result = wyhashHex("rc1");
    expect(result).toBe("0eeaba7239b72d52");
    expect(result).toHaveLength(16);
  });
});

describe("extractPreRelease", () => {
  test("extracts pre-release from standard semver", () => {
    expect(extractPreRelease("1.0.0-beta.2")).toBe("beta.2");
    expect(extractPreRelease("2.0.0-next.5")).toBe("next.5");
    expect(extractPreRelease("1.0.0-alpha.6")).toBe("alpha.6");
    expect(extractPreRelease("0.2.8-rc1")).toBe("rc1");
  });

  test("returns null for versions without pre-release", () => {
    expect(extractPreRelease("1.0.0")).toBeNull();
    expect(extractPreRelease("4.17.23")).toBeNull();
    expect(extractPreRelease("0.0.1")).toBeNull();
  });

  test("handles complex pre-release tags", () => {
    expect(extractPreRelease("1.0.0-alpha.12")).toBe("alpha.12");
    expect(extractPreRelease("1.0.0-rc.3")).toBe("rc.3");
    expect(extractPreRelease("5.0.0-alpha.12")).toBe("alpha.12");
  });
});

describe("bunCacheVersion", () => {
  test("hashes pre-release versions correctly", () => {
    expect(bunCacheVersion("1.0.0-beta.2")).toBe("1.0.0-4049f5e8f1219d89");
    expect(bunCacheVersion("2.0.0-next.5")).toBe("2.0.0-5f76f0da6d05cfd7");
    expect(bunCacheVersion("1.0.0-alpha.6")).toBe("1.0.0-32bd5ff2c1a25ef6");
    expect(bunCacheVersion("5.0.0-alpha.12")).toBe("5.0.0-93f69ee49b615ee6");
    expect(bunCacheVersion("0.2.8-rc1")).toBe("0.2.8-0eeaba7239b72d52");
  });

  test("passes through regular versions unchanged", () => {
    expect(bunCacheVersion("4.17.23")).toBe("4.17.23");
    expect(bunCacheVersion("7.29.0")).toBe("7.29.0");
    expect(bunCacheVersion("1.0.0")).toBe("1.0.0");
  });
});
