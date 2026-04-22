import fs from "node:fs/promises";
import path from "node:path";

const outputDir = path.resolve("src-tauri/generated/oauth");
const googleOutputPath = path.join(outputDir, "google-drive-client.json");
const dropboxOutputPath = path.join(outputDir, "dropbox-client.json");
const googleJsonEnv = process.env.OPENKARA_GOOGLE_DRIVE_OAUTH_CLIENT_JSON;
const googlePathEnv = process.env.OPENKARA_GOOGLE_DRIVE_OAUTH_CLIENT_JSON_PATH;
const dropboxAppKeyEnv = process.env.OPENKARA_DROPBOX_APP_KEY;
const dropboxAppSecretEnv = process.env.OPENKARA_DROPBOX_APP_SECRET;

async function ensureCleanOutputDir() {
  await fs.mkdir(outputDir, { recursive: true });
}

async function loadGoogleSourceJson() {
  if (googleJsonEnv && googleJsonEnv.trim().length > 0) {
    return googleJsonEnv;
  }

  if (googlePathEnv && googlePathEnv.trim().length > 0) {
    return fs.readFile(path.resolve(googlePathEnv), "utf8");
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

function normalizeDropboxClient() {
  if (!dropboxAppKeyEnv || dropboxAppKeyEnv.trim().length === 0) {
    return null;
  }

  return {
    app_key: dropboxAppKeyEnv.trim(),
    ...(typeof dropboxAppSecretEnv === "string" &&
    dropboxAppSecretEnv.trim().length > 0
      ? { app_secret: dropboxAppSecretEnv.trim() }
      : {}),
  };
}

async function main() {
  await ensureCleanOutputDir();
  const rawGoogle = await loadGoogleSourceJson();
  const dropbox = normalizeDropboxClient();

  if (rawGoogle === null) {
    await fs.rm(googleOutputPath, { force: true });
    console.log(
      "No bundled Google OAuth client configured; release builds will rely on runtime env vars only.",
    );
  } else {
    const normalizedGoogle = normalizeInstalledClient(rawGoogle);
    await fs.writeFile(
      googleOutputPath,
      `${JSON.stringify(normalizedGoogle)}\n`,
      "utf8",
    );
    console.log(`Prepared bundled Google OAuth client at ${googleOutputPath}`);
  }

  if (dropbox === null) {
    await fs.rm(dropboxOutputPath, { force: true });
    console.log(
      "No bundled Dropbox OAuth client configured; release builds will rely on runtime env vars only.",
    );
    return;
  }

  await fs.writeFile(dropboxOutputPath, `${JSON.stringify(dropbox)}\n`, "utf8");
  console.log(`Prepared bundled Dropbox OAuth client at ${dropboxOutputPath}`);
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
});
