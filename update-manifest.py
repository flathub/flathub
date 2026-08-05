#!/usr/bin/env python3
"""Point the extra-data sources at a new Inkdrop release.

Resolves the version from electron-builder's update feed (or takes one on the
command line), streams each .deb to compute the sha256 and byte size that
`extra-data` requires, and rewrites the manifest in place.

Flathub's own bot runs flatpak-external-data-checker and does this automatically
once the app is published. This script covers the cases the bot cannot: the
initial submission, and pinning a specific version by hand.

    ./update-manifest.py                  # follow the stable feed
    ./update-manifest.py 6.0.1            # pin a version
    ./update-manifest.py --dry-run        # print the values, touch nothing
"""

import argparse
import hashlib
import re
import sys
import urllib.request
from pathlib import Path

HERE = Path(__file__).parent
MANIFEST = HERE / "app.inkdrop.Inkdrop.yml"
METAINFO = HERE / "app.inkdrop.Inkdrop.metainfo.xml"
RELEASES = "https://dist.inkdrop.app/releases"

ARCHES = [
    ("x86_64", "amd64", "latest-linux.yml"),
    ("aarch64", "arm64", "latest-linux-arm64.yml"),
]

PRERELEASE_RE = re.compile(r"[-+](rc|alpha|beta|canary|next)", re.IGNORECASE)


def fetch_feed_version(feed):
    with urllib.request.urlopen(f"{RELEASES}/{feed}") as response:
        text = response.read().decode("utf-8")
    match = re.search(r"^version:\s*(\S+)", text, re.MULTILINE)
    if not match:
        sys.exit(f"no version found in {feed}")
    return match.group(1)


def digest(url):
    """Stream the artifact once, returning (sha256_hex, size_in_bytes)."""
    sha256 = hashlib.sha256()
    size = 0
    with urllib.request.urlopen(url) as response:
        for chunk in iter(lambda: response.read(1 << 20), b""):
            sha256.update(chunk)
            size += len(chunk)
    return sha256.hexdigest(), size


def rewrite_manifest(text, updates):
    """Replace url/sha256/size inside each extra-data block, keyed by arch.

    Deliberately line-based rather than a YAML round-trip so comments, key order
    and quoting style survive untouched.
    """
    out = []
    arch = None
    key_column = None

    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        column = len(line) - len(line.lstrip())

        if stripped == "- type: extra-data":
            # The "- " marker sits where the source's own keys begin.
            arch, key_column = None, column + 2
        elif stripped.startswith("- type:"):
            arch, key_column = None, None

        if key_column is not None:
            if arch is None and stripped.lstrip("- ") in updates:
                arch = stripped.lstrip("- ")
            elif arch is not None and column == key_column:
                # Anything deeper belongs to a nested mapping such as
                # x-checker-data, which has a `url` of its own to leave alone.
                for key, value in zip(("url", "sha256", "size"), updates[arch]):
                    if stripped.startswith(f"{key}:"):
                        line = f"{' ' * column}{key}: {value}\n"

        out.append(line)

    return "".join(out)


def warn_on_metainfo_mismatch(version):
    match = re.search(r'<release version="([^"]+)"', METAINFO.read_text())
    if match and match.group(1) != version:
        print(
            f"\nwarning: {METAINFO.name} still lists <release version=\"{match.group(1)}\">.\n"
            f"         The linter compares it against the build, so update it to {version}."
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", nargs="?", help="defaults to the version in the stable feed")
    parser.add_argument("--dry-run", action="store_true", help="print the values, write nothing")
    parser.add_argument(
        "--allow-prerelease",
        action="store_true",
        help="proceed even though Flathub stable rejects pre-releases",
    )
    args = parser.parse_args()

    version = args.version
    if version is None:
        versions = {arch: fetch_feed_version(feed) for arch, _, feed in ARCHES}
        if len(set(versions.values())) != 1:
            sys.exit(f"feeds disagree on the version: {versions}")
        version = next(iter(versions.values()))
        print(f"resolved {version} from the update feeds")

    if PRERELEASE_RE.search(version) and not args.allow_prerelease:
        sys.exit(
            f"{version} is a pre-release, which flatpak-builder-lint rejects on the stable "
            f"remote. Pass --allow-prerelease to build it locally anyway."
        )

    updates = {}
    for flatpak_arch, deb_arch, _ in ARCHES:
        url = f"{RELEASES}/inkdrop-{version}-{deb_arch}-linux.deb"
        print(f"hashing {url}")
        sha256, size = digest(url)
        print(f"  sha256: {sha256}\n  size:   {size}")
        updates[flatpak_arch] = (url, sha256, size)

    original = MANIFEST.read_text()
    updated = rewrite_manifest(original, updates)

    for url, sha256, _ in updates.values():
        if f"url: {url}" not in updated or f"sha256: {sha256}" not in updated:
            sys.exit("manifest rewrite failed — check the extra-data block layout")

    touched = sum(1 for a, b in zip(original.splitlines(), updated.splitlines()) if a != b)
    if touched > 3 * len(updates):
        sys.exit(f"manifest rewrite touched {touched} lines, expected at most {3 * len(updates)}")

    if args.dry_run:
        print(f"\n--dry-run: {MANIFEST.name} left unchanged")
    elif updated == original:
        print(f"\n{MANIFEST.name} already at {version}")
    else:
        MANIFEST.write_text(updated)
        print(f"\nupdated {MANIFEST.name} to {version}")

    warn_on_metainfo_mismatch(version)


if __name__ == "__main__":
    main()
