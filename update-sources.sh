#!/usr/bin/env bash
# Regenerates the vendored cargo and node sources for the tag in the manifest.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
manifest="$here/dev.creeperkatze.full-steam-ahead.yml"
tools="$here/flatpak-builder-tools"
raw="https://raw.githubusercontent.com/creeperkatze/full-steam-ahead"
venv="${XDG_CACHE_HOME:-$HOME/.cache}/full-steam-ahead-flatpak/venv"

tag="$(sed -n 's/^ *tag: *//p' "$manifest" | head -1)"
if [ -z "$tag" ]; then
	echo "No tag found in $manifest" >&2
	exit 1
fi
echo "Generating sources for $tag"

[ -f "$tools/cargo/flatpak-cargo-generator.py" ] ||
	git -C "$here" submodule update --init flatpak-builder-tools

locks="$(mktemp -d)"
trap 'rm -rf "$locks"' EXIT
curl -fsSL "$raw/$tag/src-tauri/Cargo.lock" -o "$locks/Cargo.lock"
curl -fsSL "$raw/$tag/pnpm-lock.yaml" -o "$locks/pnpm-lock.yaml"

[ -x "$venv/bin/pip" ] || python3 -m venv --clear "$venv"
"$venv/bin/pip" install --quiet --upgrade aiohttp PyYAML tomlkit "$tools/node"

"$venv/bin/python" "$tools/cargo/flatpak-cargo-generator.py" \
	-o "$here/cargo-sources.json" "$locks/Cargo.lock"

"$venv/bin/flatpak-node-generator" --no-requests-cache --pnpm-store-version v11 \
	-o "$here/node-sources.json" pnpm "$locks/pnpm-lock.yaml"

echo "Generated sources for $tag."
