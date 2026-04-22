import { readdir, rm } from "node:fs/promises";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, "..");
const targetRoot = path.join(projectRoot, "src-tauri", "target");

if (process.platform !== "darwin") {
  console.log("Skipping macOS bundle cleanup on non-darwin host");
  process.exit(0);
}

async function collectBundleDirectories() {
  const directories = new Set([
    path.join(targetRoot, "release", "bundle", "macos"),
  ]);

  for (const entry of await readdir(targetRoot, { withFileTypes: true })) {
    if (!entry.isDirectory() || !entry.name.endsWith("-apple-darwin")) {
      continue;
    }

    directories.add(
      path.join(targetRoot, entry.name, "release", "bundle", "macos"),
    );
  }

  return [...directories];
}

async function removeStrayDmgs(bundleDirectory) {
  let entries;
  try {
    entries = await readdir(bundleDirectory, { withFileTypes: true });
  } catch (error) {
    if (
      error &&
      typeof error === "object" &&
      "code" in error &&
      error.code === "ENOENT"
    ) {
      return [];
    }
    throw error;
  }

  const removed = [];
  for (const entry of entries) {
    if (!entry.isFile() || !entry.name.endsWith(".dmg")) {
      continue;
    }

    const dmgPath = path.join(bundleDirectory, entry.name);
    await rm(dmgPath, { force: true });
    removed.push(dmgPath);
  }

  return removed;
}

const removed = [];
for (const bundleDirectory of await collectBundleDirectories()) {
  removed.push(...(await removeStrayDmgs(bundleDirectory)));
}

if (removed.length === 0) {
  console.log("No stray macOS bundle DMGs found");
} else {
  console.log(`Removed ${removed.length} stray macOS bundle DMG(s):`);
  for (const dmgPath of removed) {
    console.log(`- ${path.relative(projectRoot, dmgPath)}`);
  }
}
