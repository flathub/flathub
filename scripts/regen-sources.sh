#!/usr/bin/env bash
# regen-sources.sh TAG
# Fetch Mortar TAG's go.mod/go.sum and lockfile from Rethunk-Tech/mortar, regenerate
# go-sources.json and node-sources.json with flatpak-go-mod / flatpak-node-generator
# (via uvx or bunx), and pin the git tag and commit in tech.rethunk.Mortar.yml.
set -euo pipefail

usage="usage: $0 TAG"
tag="${1:?$usage}"
if [ $# -ne 1 ]; then
  echo "$usage" >&2
  exit 2
fi

root="$(cd "$(dirname "$0")/.." && pwd)"
manifest="$root/tech.rethunk.Mortar.yml"
repo="https://github.com/Rethunk-Tech/mortar.git"
node_from="git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node"

commit="$(git ls-remote "$repo" "refs/tags/${tag}^{}" | awk '{print $1}')"
if [ -z "$commit" ]; then
  commit="$(git ls-remote "$repo" "refs/tags/${tag}" | awk '{print $1}')"
fi
if [ -z "$commit" ]; then
  echo "unknown tag: $tag" >&2
  exit 1
fi

work="$(mktemp -d "${TMPDIR:-/tmp}/regen-sources.XXXXXX")"
cleanup() {
  chmod -R u+w "$work" 2>/dev/null || true
  rm -rf "$work"
}
trap cleanup EXIT

# Shallow clone of the tag is how we fetch go.mod, go.sum, and the lockfile (and the
# rest of the module tree the generators need).
git clone --depth 1 --branch "$tag" "$repo" "$work/mortar"

run_uvx_or_bunx() {
  if command -v uvx >/dev/null 2>&1; then
    uvx "$@"
  elif command -v bunx >/dev/null 2>&1; then
    bunx "$@"
  else
    echo "uvx or bunx is required" >&2
    exit 1
  fi
}

# flatpak-go-mod is the Go tool documented by flatpak-builder-tools/go-modules.
# uvx/bunx are tried first; they do not ship it, so go run is the documented install.
go_ok=0
if command -v uvx >/dev/null 2>&1 && uvx flatpak-go-mod -json -out "$root" "$work/mortar"; then
  go_ok=1
elif command -v bunx >/dev/null 2>&1 && bunx flatpak-go-mod -json -out "$root" "$work/mortar"; then
  go_ok=1
else
  go run github.com/dennwc/flatpak-go-mod@latest -json -out "$root" "$work/mortar"
  go_ok=1
fi
if [ "$go_ok" -ne 1 ]; then
  echo "flatpak-go-mod failed" >&2
  exit 1
fi
if [ -f "$root/go.mod.json" ]; then
  mv "$root/go.mod.json" "$root/go-sources.json"
fi
rm -f "$root/modules.txt"

lock_kind=
lock_path=
if [ -f "$work/mortar/yarn.lock" ]; then
  lock_kind=yarn
  lock_path="$work/mortar/yarn.lock"
elif [ -f "$work/mortar/package-lock.json" ]; then
  lock_kind=npm
  lock_path="$work/mortar/package-lock.json"
elif [ -f "$work/mortar/pnpm-lock.yaml" ]; then
  lock_kind=pnpm
  lock_path="$work/mortar/pnpm-lock.yaml"
elif [ -f "$work/mortar/bun.lock" ]; then
  (cd "$work/mortar" && bun install --ignore-scripts --yarn)
  # Yarn has no lock entry for a workspace; drop those blocks so the generator can read it.
  awk 'BEGIN { RS = ""; ORS = "\n\n" } !/resolved "workspace:/' \
    "$work/mortar/yarn.lock" >"$work/yarn.lock"
  lock_kind=yarn
  lock_path="$work/yarn.lock"
else
  echo "no lockfile at $tag (tried yarn.lock, package-lock.json, pnpm-lock.yaml, bun.lock)" >&2
  exit 1
fi

if command -v uvx >/dev/null 2>&1; then
  uvx --from "$node_from" flatpak-node-generator "$lock_kind" "$lock_path" -o "$root/node-sources.json"
elif command -v bunx >/dev/null 2>&1; then
  echo "flatpak-node-generator is a Python package; install uvx to run it" >&2
  exit 1
else
  echo "uvx or bunx is required" >&2
  exit 1
fi

sed -i -E '/url: https:\/\/github\.com\/Rethunk-Tech\/mortar\.git$/{
	n
	s/^([[:space:]]*)tag: .*/\1tag: '"$tag"'/
	n
	s/^([[:space:]]*)commit: .*/\1commit: "'"$commit"'"/
}' "$manifest"
