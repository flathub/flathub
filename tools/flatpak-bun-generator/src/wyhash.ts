const MASK64 = 0xFFFFFFFFFFFFFFFFn;

const primes: bigint[] = [
  0xa0761d6478bd642fn,
  0xe7037ed1a0b428dbn,
  0x8ebc6af09c88c6e3n,
  0x589965cc75374cc3n,
  0x1d8e4e27c47d124fn,
];

function readBytes(data: Uint8Array, offset: number, count: number): bigint {
  let result = 0n;
  for (let i = 0; i < count; i++) {
    result |= BigInt(data[offset + i]) << BigInt(8 * i);
  }
  return result;
}

function read8bytesSwapped(data: Uint8Array, offset: number): bigint {
  const lo = readBytes(data, offset, 4);
  const hi = readBytes(data, offset + 4, 4);
  return ((lo << 32n) | hi) & MASK64;
}

function mum(a: bigint, b: bigint): bigint {
  const r = (a & MASK64) * (b & MASK64);
  return ((r >> 64n) ^ r) & MASK64;
}

function mix0(a: bigint, b: bigint, seed: bigint): bigint {
  return mum(
    (a ^ seed ^ primes[0]) & MASK64,
    (b ^ seed ^ primes[1]) & MASK64
  );
}

function mix1(a: bigint, b: bigint, seed: bigint): bigint {
  return mum(
    (a ^ seed ^ primes[2]) & MASK64,
    (b ^ seed ^ primes[3]) & MASK64
  );
}

export function wyhash(seed: bigint, input: Uint8Array): bigint {
  let s = seed & MASK64;
  let msgLen = 0;

  const alignedLen = input.length - (input.length % 32);
  for (let off = 0; off < alignedLen; off += 32) {
    s = (mix0(
      readBytes(input, off, 8),
      readBytes(input, off + 8, 8),
      s,
    ) ^ mix1(
      readBytes(input, off + 16, 8),
      readBytes(input, off + 24, 8),
      s,
    )) & MASK64;
  }
  msgLen += alignedLen;

  const rem = input.subarray(alignedLen);
  const remLen = rem.length;

  switch (remLen) {
    case 0:
      break;
    case 1:
      s = mix0(readBytes(rem, 0, 1), primes[4], s);
      break;
    case 2:
      s = mix0(readBytes(rem, 0, 2), primes[4], s);
      break;
    case 3:
      s = mix0((readBytes(rem, 0, 2) << 8n) | readBytes(rem, 2, 1), primes[4], s);
      break;
    case 4:
      s = mix0(readBytes(rem, 0, 4), primes[4], s);
      break;
    case 5:
      s = mix0((readBytes(rem, 0, 4) << 8n) | readBytes(rem, 4, 1), primes[4], s);
      break;
    case 6:
      s = mix0((readBytes(rem, 0, 4) << 16n) | readBytes(rem, 4, 2), primes[4], s);
      break;
    case 7:
      s = mix0((readBytes(rem, 0, 4) << 24n) | (readBytes(rem, 4, 2) << 8n) | readBytes(rem, 6, 1), primes[4], s);
      break;
    case 8:
      s = mix0(read8bytesSwapped(rem, 0), primes[4], s);
      break;
    case 9:
      s = mix0(read8bytesSwapped(rem, 0), readBytes(rem, 8, 1), s);
      break;
    case 10:
      s = mix0(read8bytesSwapped(rem, 0), readBytes(rem, 8, 2), s);
      break;
    case 11:
      s = mix0(read8bytesSwapped(rem, 0), (readBytes(rem, 8, 2) << 8n) | readBytes(rem, 10, 1), s);
      break;
    case 12:
      s = mix0(read8bytesSwapped(rem, 0), readBytes(rem, 8, 4), s);
      break;
    case 13:
      s = mix0(read8bytesSwapped(rem, 0), (readBytes(rem, 8, 4) << 8n) | readBytes(rem, 12, 1), s);
      break;
    case 14:
      s = mix0(read8bytesSwapped(rem, 0), (readBytes(rem, 8, 4) << 16n) | readBytes(rem, 12, 2), s);
      break;
    case 15:
      s = mix0(read8bytesSwapped(rem, 0), (readBytes(rem, 8, 4) << 24n) | (readBytes(rem, 12, 2) << 8n) | readBytes(rem, 14, 1), s);
      break;
    case 16:
      s = mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s);
      break;
    case 17:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(readBytes(rem, 16, 1), primes[4], s)) & MASK64;
      break;
    case 18:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(readBytes(rem, 16, 2), primes[4], s)) & MASK64;
      break;
    case 19:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1((readBytes(rem, 16, 2) << 8n) | readBytes(rem, 18, 1), primes[4], s)) & MASK64;
      break;
    case 20:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(readBytes(rem, 16, 4), primes[4], s)) & MASK64;
      break;
    case 21:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1((readBytes(rem, 16, 4) << 8n) | readBytes(rem, 20, 1), primes[4], s)) & MASK64;
      break;
    case 22:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1((readBytes(rem, 16, 4) << 16n) | readBytes(rem, 20, 2), primes[4], s)) & MASK64;
      break;
    case 23:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1((readBytes(rem, 16, 4) << 24n) | (readBytes(rem, 20, 2) << 8n) | readBytes(rem, 22, 1), primes[4], s)) & MASK64;
      break;
    case 24:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), primes[4], s)) & MASK64;
      break;
    case 25:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), readBytes(rem, 24, 1), s)) & MASK64;
      break;
    case 26:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), readBytes(rem, 24, 2), s)) & MASK64;
      break;
    case 27:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), (readBytes(rem, 24, 2) << 8n) | readBytes(rem, 26, 1), s)) & MASK64;
      break;
    case 28:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), readBytes(rem, 24, 4), s)) & MASK64;
      break;
    case 29:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), (readBytes(rem, 24, 4) << 8n) | readBytes(rem, 28, 1), s)) & MASK64;
      break;
    case 30:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), (readBytes(rem, 24, 4) << 16n) | readBytes(rem, 28, 2), s)) & MASK64;
      break;
    case 31:
      s = (mix0(read8bytesSwapped(rem, 0), read8bytesSwapped(rem, 8), s) ^ mix1(read8bytesSwapped(rem, 16), (readBytes(rem, 24, 4) << 24n) | (readBytes(rem, 28, 2) << 8n) | readBytes(rem, 30, 1), s)) & MASK64;
      break;
  }

  msgLen += remLen;

  return mum((s ^ BigInt(msgLen)) & MASK64, primes[4]);
}

export function wyhashHex(input: string): string {
  const bytes = new TextEncoder().encode(input);
  const hash = wyhash(0n, bytes);
  return hash.toString(16).padStart(16, "0");
}

export function extractPreRelease(version: string): string | null {
  const match = version.match(/^\d+\.\d+\.\d+-(.+)$/);
  return match ? match[1] : null;
}

export function bunCacheVersion(version: string): string {
  const pre = extractPreRelease(version);
  if (!pre) return version;

  const majorMinorPatch = version.slice(0, version.indexOf("-"));
  return `${majorMinorPatch}-${wyhashHex(pre)}`;
}
