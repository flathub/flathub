import { createHash } from "node:crypto";

function githubHeaders() {
  const headers = {
    Accept: "application/vnd.github+json",
    "User-Agent": "openkara-release-automation",
  };

  if (process.env.GITHUB_TOKEN) {
    headers.Authorization = `Bearer ${process.env.GITHUB_TOKEN}`;
  }

  return headers;
}

export async function fetchReleaseByTag({ owner, repo, tag }) {
  const releaseByTagUrl = `https://api.github.com/repos/${owner}/${repo}/releases/tags/${tag}`;
  const response = await fetch(releaseByTagUrl, {
    headers: githubHeaders(),
  });

  if (response.ok) {
    return response.json();
  }

  if (response.status !== 404) {
    throw new Error(
      `failed to fetch release ${owner}/${repo}@${tag}: ${response.status} ${response.statusText}`,
    );
  }

  const releasesResponse = await fetch(
    `https://api.github.com/repos/${owner}/${repo}/releases?per_page=100`,
    {
      headers: githubHeaders(),
    },
  );

  if (!releasesResponse.ok) {
    throw new Error(
      `failed to list releases ${owner}/${repo}: ${releasesResponse.status} ${releasesResponse.statusText}`,
    );
  }

  const releases = await releasesResponse.json();
  const release = releases.find((candidate) => candidate.tag_name === tag);
  if (!release) {
    throw new Error(
      `failed to fetch release ${owner}/${repo}@${tag}: ${response.status} ${response.statusText}`,
    );
  }

  return release;
}

export async function sha256ForUrl(url) {
  const response = await fetch(url, {
    headers: githubHeaders(),
    redirect: "follow",
  });

  if (!response.ok) {
    throw new Error(
      `failed to download ${url}: ${response.status} ${response.statusText}`,
    );
  }

  const hash = createHash("sha256");
  for await (const chunk of response.body) {
    hash.update(chunk);
  }
  return hash.digest("hex");
}

export function requireReleaseAsset(release, name) {
  const asset = release.assets.find((candidate) => candidate.name === name);
  if (!asset) {
    throw new Error(
      `release ${release.tag_name} does not contain asset ${name}`,
    );
  }
  return asset;
}

export function releaseDateISO(release) {
  const published = release.published_at ?? release.created_at;
  if (!published) {
    throw new Error(`release ${release.tag_name} does not have a publish date`);
  }

  return published.slice(0, 10);
}

export function parseArgs(argv) {
  const args = {};

  for (let index = 0; index < argv.length; index += 1) {
    const current = argv[index];
    if (!current.startsWith("--")) {
      continue;
    }

    const key = current.slice(2);
    const next = argv[index + 1];
    if (!next || next.startsWith("--")) {
      args[key] = "true";
      continue;
    }

    args[key] = next;
    index += 1;
  }

  return args;
}

export function requireArg(args, key) {
  const value = args[key];
  if (!value) {
    throw new Error(`missing required argument --${key}`);
  }
  return value;
}
