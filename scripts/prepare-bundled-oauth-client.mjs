import fs from "node:fs/promises";
import path from "node:path";

const outputDir = path.resolve("src-tauri/generated/oauth");
const outputPath = path.join(outputDir, "google-drive-client.json");
const jsonEnv = process.env.OPENKARA_GOOGLE_DRIVE_OAUTH_CLIENT_JSON;
const pathEnv = process.env.OPENKARA_GOOGLE_DRIVE_OAUTH_CLIENT_JSON_PATH;

async function ensureCleanOutputDir() {
  await fs.mkdir(outputDir, { recursive: true });
}

async function loadSourceJson() {
  if (jsonEnv && jsonEnv.trim().length > 0) {
    return jsonEnv;
  }

  if (pathEnv && pathEnv.trim().length > 0) {
    return fs.readFile(path.resolve(pathEnv), "utf8");
  }

  return null;
}

function normalizeInstalledClient(raw) {
  const parsed = JSON.parse(raw);
  const installed = parsed?.installed;
  const clientId = installed?.client_id;
  const clientSecret = installed?.client_secret;

  if (typeof clientId !== "string" || clientId.trim().length === 0) {
    throw new Error(
      "Google OAuth client JSON must include installed.client_id for Desktop app credentials.",
    );
  }

  return {
    installed: {
      client_id: clientId.trim(),
      ...(typeof clientSecret === "string" && clientSecret.trim().length > 0
        ? { client_secret: clientSecret.trim() }
        : {}),
    },
  };
}

async function main() {
  await ensureCleanOutputDir();
  const raw = await loadSourceJson();

  if (raw === null) {
    await fs.rm(outputPath, { force: true });
    console.log(
      "No bundled Google OAuth client configured; release builds will rely on runtime env vars only.",
    );
    return;
  }

  const normalized = normalizeInstalledClient(raw);
  await fs.writeFile(outputPath, `${JSON.stringify(normalized)}\n`, "utf8");
  console.log(`Prepared bundled Google OAuth client at ${outputPath}`);
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
});
