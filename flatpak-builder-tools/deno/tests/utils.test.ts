// LICENSE = MIT
// deno-lint-ignore no-import-prefix
import { assert, assertEquals, assertMatch } from "jsr:@std/assert@1.0.16";
import {
  base64ToHex,
  sha256,
  shortHash,
  shouldHash,
  splitOnce,
  urlSegments,
} from "../src/utils.ts";

Deno.test("sha256 produces correct hash", async () => {
  const hash = await sha256("hello world");
  assertEquals(
    hash,
    "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9",
  );
});

Deno.test("base64ToHex converts base64 to hex", () => {
  // "hello" in base64 is "aGVsbG8="
  // "hello" in hex is "68656c6c6f"
  assertEquals(base64ToHex("aGVsbG8="), "68656c6c6f");
});

Deno.test("splitOnce splits string correctly (left)", () => {
  assertEquals(splitOnce("foo:bar:baz", ":"), ["foo", "bar:baz"]);
  assertEquals(splitOnce("foo", ":"), ["foo"]);
});

Deno.test("splitOnce splits string correctly (right)", () => {
  assertEquals(splitOnce("foo:bar:baz", ":", "right"), ["foo:bar", "baz"]);
  assertEquals(splitOnce("foo", ":", "right"), ["foo"]);
});

Deno.test("shouldHash returns true for forbidden or long file names", () => {
  assert(shouldHash("ThisIsUppercase.txt"));
  assert(shouldHash("file?name.txt"));
  assert(shouldHash("a".repeat(31)));
  assert(shouldHash(""));
});

Deno.test("shouldHash returns false for safe short lowercase names", () => {
  assert(!shouldHash("file.txt"));
  assert(!shouldHash("abc"));
});

Deno.test("shortHash returns hashed filename for forbidden/long names", async () => {
  const result = await shortHash("ThisIsUppercase.txt");
  assertMatch(result, /^#thisisuppercase_[a-f0-9]{5}\.txt$/);
  const result2 = await shortHash("file?name.txt");
  assertMatch(result2, /^#file_[a-f0-9]{5}.txt$/);
  const result3 = await shortHash("a".repeat(40));
  assertMatch(result3, /^#aaaaaaaaaaaaaaaaaaaa_[a-f0-9]{5}$/);
  const result4 = await shortHash("file<name.ts");
  assertMatch(result4, /^#file_name_[a-f0-9]{5}.ts$/);
  const result5 = await shortHash(
    "unstable_get_network_address.ts",
  );
  assertEquals(result5, "#unstable_get_network_b61b7.ts");
});

Deno.test("shortHash returns hashed filename for empty string", async () => {
  const result = await shortHash("");
  assertMatch(result, /^#[a-f0-9]{7}$/);
});

Deno.test("urlSegments splits URL path into segments", () => {
  assertEquals(
    urlSegments("https://example.com/foo/bar/baz.txt"),
    ["foo", "bar", "baz.txt"],
  );
  assertEquals(
    urlSegments(new URL("https://example.com/a/b/c")),
    ["a", "b", "c"],
  );
  assertEquals(
    urlSegments("https://example.com/"),
    [""],
  );
});
