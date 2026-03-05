import { describe, expect, test } from "bun:test";
import {
  sha256,
  base64ToHex,
  splitOnce,
  stripJsoncTrailingCommas,
} from "../src/utils.ts";

describe("sha256", () => {
  test("hashes known string correctly", async () => {
    const hash = await sha256("hello world");
    expect(hash).toBe(
      "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"
    );
  });

  test("hashes empty string", async () => {
    const hash = await sha256("");
    expect(hash).toBe(
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    );
  });
});

describe("base64ToHex", () => {
  test("converts base64 to hex", () => {
    expect(base64ToHex("aGVsbG8=")).toBe("68656c6c6f");
  });

  test("handles longer base64 strings", () => {
    expect(base64ToHex("SGVsbG8sIFdvcmxkIQ==")).toBe(
      "48656c6c6f2c20576f726c6421"
    );
  });
});

describe("splitOnce", () => {
  test("splits on first occurrence (left)", () => {
    expect(splitOnce("a-b-c", "-")).toEqual(["a", "b-c"]);
  });

  test("splits on last occurrence (right)", () => {
    expect(splitOnce("a-b-c", "-", "right")).toEqual(["a-b", "c"]);
  });

  test("returns original if separator not found", () => {
    expect(splitOnce("abc", "-")).toEqual(["abc"]);
  });

  test("handles scoped packages", () => {
    expect(splitOnce("sha512-abc123", "-")).toEqual(["sha512", "abc123"]);
  });
});

describe("stripJsoncTrailingCommas", () => {
  test("strips trailing commas before closing brace", () => {
    const input = '{"a": 1, "b": 2,}';
    const result = stripJsoncTrailingCommas(input);
    expect(JSON.parse(result)).toEqual({ a: 1, b: 2 });
  });

  test("strips trailing commas before closing bracket", () => {
    const input = "[1, 2, 3,]";
    const result = stripJsoncTrailingCommas(input);
    expect(JSON.parse(result)).toEqual([1, 2, 3]);
  });

  test("handles nested trailing commas", () => {
    const input = '{"a": {"b": 1,}, "c": [1, 2,],}';
    const result = stripJsoncTrailingCommas(input);
    expect(JSON.parse(result)).toEqual({ a: { b: 1 }, c: [1, 2] });
  });

  test("leaves valid JSON unchanged", () => {
    const input = '{"a": 1, "b": 2}';
    const result = stripJsoncTrailingCommas(input);
    expect(JSON.parse(result)).toEqual({ a: 1, b: 2 });
  });
});
