export async function sha256(text: string): Promise<string> {
  const data = new TextEncoder().encode(text);
  const hashBuffer = await crypto.subtle.digest("SHA-256", data);
  return Array.from(new Uint8Array(hashBuffer))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

export function base64ToHex(base64String: string): string {
  const binaryData = Uint8Array.from(atob(base64String), (c) =>
    c.charCodeAt(0)
  );
  return Array.from(binaryData)
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

export function splitOnce(
  str: string,
  separator: string,
  dir: "left" | "right" = "left"
): [string] | [string, string] {
  const idx =
    dir === "left" ? str.indexOf(separator) : str.lastIndexOf(separator);
  if (idx === -1) return [str];
  return [str.slice(0, idx), str.slice(idx + separator.length)];
}

export function stripJsoncTrailingCommas(text: string): string {
  return text.replace(/,\s*([\]}])/g, "$1");
}
