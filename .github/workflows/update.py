#!/usr/bin/env python3
import os
import sys
import json
import re
import hashlib
import datetime
import urllib.request

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

API_URL = "https://api.github.com/repos/HMCL-dev/HMCL/releases/latest"
HEADERS = {
    "User-Agent": "FlatHMCL-Sync-Bot",
    "Accept": "application/vnd.github+json"
}

token = os.environ.get("GITHUB_TOKEN")
if token:
    HEADERS["Authorization"] = f"Bearer {token}"

print("[INFO] Fetching upstream HMCL release from GitHub API...")
try:
    req = urllib.request.Request(API_URL, headers=HEADERS)
    with urllib.request.urlopen(req, timeout=15) as resp:
        release_data = json.loads(resp.read().decode())
except Exception as e:
    print(f"[ERROR] Failed to query GitHub API: {e}", file=sys.stderr)
    sys.exit(1)

tag_name = release_data.get("tag_name", "")
version = tag_name.lstrip("v")
assets = release_data.get("assets", [])

target_asset = None
for asset in assets:
    name = asset.get("name", "")
    if name.endswith(".jar") and "HMCL" in name:
        target_asset = asset
        break

if not target_asset:
    print("[ERROR] No HMCL .jar found in release assets.", file=sys.stderr)
    sys.exit(1)

jar_url = target_asset.get("browser_download_url")
digest = target_asset.get("digest") or ""

if digest.startswith("sha256:"):
    jar_sha256 = digest[7:]
else:
    print("[INFO] Calculating SHA256 from stream...")
    req_jar = urllib.request.Request(jar_url, headers={"User-Agent": "FlatHMCL-Sync-Bot"})
    with urllib.request.urlopen(req_jar, timeout=30) as resp_jar:
        h = hashlib.sha256()
        while chunk := resp_jar.read(65536):
            h.update(chunk)
        jar_sha256 = h.hexdigest()

print(f"[INFO] Upstream Target Tag : {tag_name}")
print(f"[INFO] Upstream Version    : {version}")
print(f"[INFO] Target Download URL : {jar_url}")
print(f"[INFO] Target SHA256       : {jar_sha256}")

# 1. Update Manifest
manifest_path = os.path.join(REPO_ROOT, "io.github.theoninesixy.HMCL.yml")
if os.path.exists(manifest_path):
    with open(manifest_path, "r", encoding="utf-8") as f:
        manifest_content = f.read()

    new_manifest = re.sub(
        r'https://github\.com/HMCL-dev/HMCL/releases/download/[^\s"\']+',
        jar_url,
        manifest_content
    )
    new_manifest = re.sub(
        r'(sha256:\s*)[a-f0-9]{64}',
        f'\\g<1>{jar_sha256}',
        new_manifest
    )

    if new_manifest != manifest_content:
        with open(manifest_path, "w", encoding="utf-8") as f:
            f.write(new_manifest)
        print(f"[INFO] Updated {manifest_path} to version {version}.")
    else:
        print(f"[INFO] {manifest_path} is already up to date.")
else:
    print(f"[WARN] File not found: {manifest_path}")

# 2. Update Metainfo
metainfo_path = os.path.join(REPO_ROOT, "io.github.theoninesixy.HMCL.metainfo.xml")
if os.path.exists(metainfo_path):
    with open(metainfo_path, "r", encoding="utf-8") as f:
        metainfo_content = f.read()

    release_date = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d")
    new_metainfo = re.sub(
        r'<release version="[^"]+" date="[^"]+">',
        f'<release version="{version}" date="{release_date}">',
        metainfo_content
    )
    new_metainfo = re.sub(
        r'(<p>同步 HMCL 上游官方版本 )[^<]+( 发布。<\/p>)',
        f'\\g<1>{version}\\g<2>',
        new_metainfo
    )

    if new_metainfo != metainfo_content:
        with open(metainfo_path, "w", encoding="utf-8") as f:
            f.write(new_metainfo)
        print(f"[INFO] Updated {metainfo_path} to version {version}.")
    else:
        print(f"[INFO] {metainfo_path} is already up to date.")
else:
    print(f"[WARN] File not found: {metainfo_path}")