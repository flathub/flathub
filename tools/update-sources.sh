#!/bin/bash
# Regenerate cargo-sources.json and node-sources.json for a WebClients tag or commit.
#
# Requires git, node >= 24, flatpak-node-generator on PATH and
# FLATPAK_CARGO_GENERATOR pointing at flatpak-cargo-generator.py.
#
#   tools/update-sources.sh proton-authenticator@1.1.7
set -euo pipefail

ref=${1:?usage: $0 <tag-or-commit>}
repo=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

git clone --quiet --filter=blob:none --no-checkout https://github.com/ProtonMail/WebClients.git "$work/src"
git -C "$work/src" fetch --quiet origin "$ref"
git -C "$work/src" checkout --quiet FETCH_HEAD

cd "$work/src"
python3 "$repo/tools/strip-private-registry-lockfile.py" yarn.lock yarn.lock.public > /dev/null
mv yarn.lock.public yarn.lock
python3 "$repo/tools/clean-yarnrc.py"

# A focused install fetches exactly the packages the authenticator needs
YARN_ENABLE_GLOBAL_CACHE=0 YARN_CACHE_FOLDER="$work/cache" YARN_ENABLE_SCRIPTS=0 \
YARN_ENABLE_IMMUTABLE_INSTALLS=0 YARN_ENABLE_TELEMETRY=0 \
    node .yarn/releases/yarn-*.cjs workspaces focus proton-authenticator

python3 "$repo/tools/prune-node-sources.py" lock yarn.lock "$work/cache" > "$work/yarn.lock"
flatpak-node-generator yarn "$work/yarn.lock" -o "$repo/node-sources.json"
python3 "$repo/tools/prune-node-sources.py" sources "$repo/node-sources.json"

python3 "${FLATPAK_CARGO_GENERATOR:?}" applications/authenticator/src-tauri/Cargo.lock \
    -o "$repo/cargo-sources.json"
