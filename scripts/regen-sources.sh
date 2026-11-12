#!/usr/bin/env bash
# regen-sources.sh TAG
# Regenerate the Flathub submission files for Mortar TAG by running the render.sh the tag itself ships
# (build/linux/flathub/render.sh), so the output is exactly what that release attaches as its flathub-* files.
# Needs network, git, go, bun, jq and uvx.
set -euo pipefail

usage="usage: $0 TAG"
if [ $# -ne 1 ]; then
  echo "$usage" >&2
  exit 2
fi
tag="$1"

root="$(cd "$(dirname "$0")/.." && pwd)"
repo="https://github.com/Rethunk-Tech/mortar.git"

work="$(mktemp -d "${TMPDIR:-/var/tmp}/regen-sources.XXXXXX")"
cleanup() {
  case "$work" in
    */regen-sources.*) chmod -R u+w "$work" && rm -rf "$work" ;;
  esac
}
trap cleanup EXIT

git clone --quiet --depth 1 --branch "$tag" "$repo" "$work/mortar"
# render.sh sorts the Go module files with the ambient locale; the committed lists are C-ordered.
LC_ALL=C "$work/mortar/build/linux/flathub/render.sh" "$work/out"
cp "$work"/out/{tech.rethunk.Mortar.yml,go-sources.json,node-sources.json,yarn.lock} "$root/"
