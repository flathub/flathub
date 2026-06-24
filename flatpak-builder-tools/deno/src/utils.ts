// LICENSE = MIT
// deno-lint-ignore-file no-import-prefix

import { encodeHex } from "jsr:@std/encoding@1/hex";
import { decodeBase64 } from "jsr:@std/encoding@1/base64";

export async function sha256(text: string): Promise<string> {
  const encoder = new TextEncoder();
  const data = encoder.encode(text);
  const hashBuffer = await crypto.subtle.digest("SHA-256", data);
  // Convert buffer to hex string
  return Array.from(new Uint8Array(hashBuffer))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

/**
 * Converts a Base64 encoded string to its hexadecimal representation
 *
 * @param base64String The Base64 encoded string.
 * @returns The hexadecimal representation of the decoded string.
 */
export function base64ToHex(base64String: string): string {
  // Step 1: Base64 decode the string into a Uint8Array.
  const binaryData: Uint8Array = decodeBase64(base64String);
  // Step 2: Convert the Uint8Array (raw binary data) to a hexadecimal string.
  const hexString: string = encodeHex(binaryData);
  return hexString;
}

export function splitOnce(
  str: string,
  separator: string,
  dir: "left" | "right" = "left",
) {
  const idx = dir === "left"
    ? str.indexOf(separator)
    : str.lastIndexOf(separator);
  if (idx === -1) return [str];
  return [str.slice(0, idx), str.slice(idx + separator.length)];
}

const FORBIDDEN_CHARS = new Set([
  "?",
  "<",
  ">",
  ":",
  "*",
  "|",
  "\\",
  ":",
  '"',
  "'",
  "/",
]);

// https://github.com/denoland/deno_cache_dir/blob/0b2dbb2553019dd829d71665bed7f48f610b64f0/rs_lib/src/local.rs#L594
export function hasForbiddenChars(segment: string): boolean {
  for (const c of segment) {
    const isUppercase = /[A-Z]/.test(c);
    if (FORBIDDEN_CHARS.has(c) || isUppercase) {
      // do not allow uppercase letters in order to make this work
      // the same on case insensitive file systems
      return true;
    }
  }
  return false;
}

// https://github.com/denoland/deno_cache_dir/blob/0b2dbb2553019dd829d71665bed7f48f610b64f0/rs_lib/src/local.rs#L651
export function shouldHash(fileName: string): boolean {
  return fileName.length === 0 ||
    fileName.length > 30 ||
    hasForbiddenChars(fileName);
}

// https://github.com/denoland/deno_cache_dir/blob/0b2dbb2553019dd829d71665bed7f48f610b64f0/rs_lib/src/local.rs#L621
export async function shortHash(fileName: string): Promise<string> {
  const hash = await sha256(fileName);
  const MAX_LENGTH = 20;
  let sub = "";
  let count = 0;
  for (const c of fileName) {
    if (count >= MAX_LENGTH) break;
    if (c === "?") break;
    if (FORBIDDEN_CHARS.has(c)) {
      sub += "_";
    } else {
      sub += c.toLowerCase();
    }
    count++;
  }

  sub = splitOnce(sub, ".", "right")[0];
  let ext = splitOnce(fileName, ".", "right").at(1);
  ext = ext ? `.${ext}` : "";

  if (sub.length === 0) {
    return `#${hash.slice(0, 7)}${ext}`;
  } else {
    return `#${sub}_${hash.slice(0, 5)}${ext}`;
  }
}

export function urlSegments(url: string | URL) {
  return new URL(url).pathname.replace(/^\//, "").split("/");
}
