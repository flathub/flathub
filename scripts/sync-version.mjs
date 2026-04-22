import { readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, "..");

const packageJsonPath = path.join(projectRoot, "package.json");
const cargoTomlPath = path.join(projectRoot, "src-tauri", "Cargo.toml");
const tauriConfigPath = path.join(projectRoot, "src-tauri", "tauri.conf.json");

const packageJson = JSON.parse(await readFile(packageJsonPath, "utf8"));
const version = packageJson.version;

if (
  typeof version !== "string" ||
  !/^\d+\.\d+\.\d+([-.][0-9A-Za-z.-]+)?$/.test(version)
) {
  throw new Error(`Invalid package.json version: ${String(version)}`);
}

const updates = [];

const cargoTomlContent = await readFile(cargoTomlPath, "utf8");
const nextCargoTomlContent = cargoTomlContent.replace(
  /^version\s*=\s*"[^"]+"$/m,
  `version = "${version}"`,
);

if (cargoTomlContent !== nextCargoTomlContent) {
  await writeFile(cargoTomlPath, nextCargoTomlContent, "utf8");
  updates.push("src-tauri/Cargo.toml");
}

const tauriConfig = JSON.parse(await readFile(tauriConfigPath, "utf8"));
if (tauriConfig.version !== version) {
  tauriConfig.version = version;
  await writeFile(
    tauriConfigPath,
    `${JSON.stringify(tauriConfig, null, 2)}\n`,
    "utf8",
  );
  updates.push("src-tauri/tauri.conf.json");
}

if (updates.length === 0) {
  console.log(`Version already synced: ${version}`);
} else {
  console.log(`Synced version ${version} to: ${updates.join(", ")}`);
}
