import {
  cpSync,
  existsSync,
  mkdtempSync,
  mkdirSync,
  readFileSync,
  readdirSync,
  realpathSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { join } from "node:path";
import os from "node:os";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const ORT_VERSION = "1.23.2";
const ROOT_DIR = fileURLToPath(new URL("..", import.meta.url));
const STAGING_DIR = join(ROOT_DIR, "src-tauri", "generated", "onnxruntime");
const MANIFEST_PATH = join(STAGING_DIR, "manifest.json");

const TARGET_CONFIG = {
  "aarch64-apple-darwin": {
    archiveName: `onnxruntime-osx-arm64-${ORT_VERSION}.tgz`,
    outputName: "libonnxruntime.dylib",
    manifestTarget: "aarch64-apple-darwin",
  },
  "x86_64-apple-darwin": {
    archiveName: `onnxruntime-osx-x86_64-${ORT_VERSION}.tgz`,
    outputName: "libonnxruntime.dylib",
    manifestTarget: "x86_64-apple-darwin",
  },
  "x86_64-unknown-linux-gnu": {
    archiveName: `onnxruntime-linux-x64-${ORT_VERSION}.tgz`,
    outputName: "libonnxruntime.so",
    manifestTarget: "x86_64-unknown-linux-gnu",
  },
  "aarch64-unknown-linux-gnu": {
    archiveName: `onnxruntime-linux-aarch64-${ORT_VERSION}.tgz`,
    outputName: "libonnxruntime.so",
    manifestTarget: "aarch64-unknown-linux-gnu",
  },
  "x86_64-pc-windows-msvc": {
    archiveName: `onnxruntime-win-x64-${ORT_VERSION}.zip`,
    outputName: "onnxruntime.dll",
    manifestTarget: "x86_64-pc-windows-msvc",
  },
};

function runtimeMatcher(outputName) {
  if (outputName === "onnxruntime.dll") {
    return (fileName) => fileName === outputName;
  }

  if (outputName.endsWith(".dylib")) {
    return (fileName) =>
      fileName.startsWith("libonnxruntime") &&
      fileName.endsWith(".dylib") &&
      !fileName.includes("providers");
  }

  return (fileName) =>
    fileName.startsWith("libonnxruntime.so") &&
    !fileName.includes("providers") &&
    !fileName.endsWith(".a");
}

function parseArgs(argv) {
  const parsed = {};

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === "--target") {
      parsed.target = argv[index + 1];
      index += 1;
    }
  }

  return parsed;
}

function defaultTargetForHost() {
  if (process.platform === "darwin") {
    return process.arch === "arm64"
      ? "aarch64-apple-darwin"
      : "x86_64-apple-darwin";
  }

  if (process.platform === "linux") {
    return process.arch === "arm64"
      ? "aarch64-unknown-linux-gnu"
      : "x86_64-unknown-linux-gnu";
  }

  if (process.platform === "win32") {
    return "x86_64-pc-windows-msvc";
  }

  throw new Error(`unsupported host platform '${process.platform}'`);
}

function walkFiles(dirPath, files = []) {
  for (const entry of readdirSync(dirPath, { withFileTypes: true })) {
    const fullPath = join(dirPath, entry.name);
    if (entry.isDirectory()) {
      walkFiles(fullPath, files);
      continue;
    }

    if (entry.isFile() || entry.isSymbolicLink()) {
      files.push(fullPath);
    }
  }

  return files;
}

function readManifest() {
  if (!existsSync(MANIFEST_PATH)) {
    return null;
  }

  return JSON.parse(readFileSync(MANIFEST_PATH, "utf8"));
}

function ensureSystemTool(toolName) {
  try {
    execFileSync(toolName, ["--version"], { stdio: "ignore" });
  } catch {
    throw new Error(`required tool '${toolName}' is not installed`);
  }
}

const args = parseArgs(process.argv.slice(2));
const targetTriple =
  args.target ?? process.env.OPENKARA_ORT_TARGET ?? defaultTargetForHost();
const config = TARGET_CONFIG[targetTriple];

if (!config) {
  throw new Error(`unsupported target '${targetTriple}'`);
}

const manifest = readManifest();
const stagedRuntimePath = join(STAGING_DIR, config.outputName);
if (
  manifest?.version === ORT_VERSION &&
  manifest?.target === config.manifestTarget &&
  existsSync(stagedRuntimePath)
) {
  console.log(`ONNX Runtime already prepared at ${stagedRuntimePath}`);
  process.exit(0);
}

ensureSystemTool("tar");

const tempRoot = mkdtempSync(join(os.tmpdir(), "openkara-ort-"));
const archivePath = join(tempRoot, config.archiveName);
const extractedDir = join(tempRoot, "extracted");
mkdirSync(extractedDir, { recursive: true });

try {
  const archiveUrl = `https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/${config.archiveName}`;
  console.log(`Downloading ${archiveUrl}`);

  const response = await fetch(archiveUrl);
  if (!response.ok) {
    throw new Error(
      `failed to download ${archiveUrl}: ${response.status} ${response.statusText}`,
    );
  }

  const archiveBytes = Buffer.from(await response.arrayBuffer());
  writeFileSync(archivePath, archiveBytes);
  execFileSync("tar", ["-xf", archivePath, "-C", extractedDir], {
    stdio: "inherit",
  });

  const runtimeCandidate = walkFiles(extractedDir).find((filePath) =>
    runtimeMatcher(config.outputName)(filePath.split(/[\\/]/).pop() ?? ""),
  );
  if (!runtimeCandidate) {
    throw new Error(
      `failed to locate ${config.outputName} inside ${config.archiveName}`,
    );
  }

  rmSync(STAGING_DIR, { force: true, recursive: true });
  mkdirSync(STAGING_DIR, { recursive: true });
  cpSync(realpathSync(runtimeCandidate), stagedRuntimePath);
  writeFileSync(
    MANIFEST_PATH,
    JSON.stringify(
      {
        version: ORT_VERSION,
        target: config.manifestTarget,
        sourceArchive: config.archiveName,
      },
      null,
      2,
    ) + "\n",
  );

  console.log(`Prepared ONNX Runtime at ${stagedRuntimePath}`);
} finally {
  rmSync(tempRoot, { force: true, recursive: true });
}
