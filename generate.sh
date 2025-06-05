#!/usr/bin/env bash
set -e

# must be tags for now
# (if you want to use sth else, you need to read this script and modify it accordingly)
# DESKTOP_CHECKOUT=v1.57.0
DESKTOP_CHECKOUT=0c8da00c6993814133aa7a5e62467c1a2b4dc78f

# this script needs:
# - that you have run setup.sh before
# - python3, nodejs 20
# - jq
# - flatpak-node-generator (setup.sh installs this for you)
# you can call "nix develop" to install those dependencies if you are doing this on nix

export PATH=$PATH:$HOME/.local/bin

source working_dir/.venv/bin/activate

# git checkout & print hashes
# ===========================================================
echo "[git checkout desktop]"
cd working_dir/deltachat-desktop
git fetch --all
git fetch --all --tags
git reset --hard
git checkout $DESKTOP_CHECKOUT
git clean -d -x -f
DESKTOP_COMMIT_HASH=$(git rev-parse HEAD)
cd -

# generate sources
# ===========================================================
echo "[rust build dependencies]"
python3 working_dir/flatpak-builder-tools/cargo/flatpak-cargo-generator.py \
    -o generated/sources-rust.json \
    working_dir/deltachat-desktop/Cargo.lock

echo "[desktop build dependencies]"

# start proxy registry that records the packages that are fetched
node tool_record.mjs &
PID_RECORD=$!

cd working_dir/deltachat-desktop
pnpm config set registry http://localhost:3000 --location project
rm -r $(pwd)/.pnpm-store || true
pnpm config set store-dir $(pwd)/.pnpm-store --location project
echo "[desktop deps: ignore other architectures]"
# desktop modify package json to exclude all unused architectures
# the temporary file `package.new.json` is nessesary because jq does not support in place editing of files.
jq ".pnpm.supportedArchitectures.os = [\"linux\"] | .pnpm.supportedArchitectures.cpu = [\"x64\", \"arm64\"]" package.json > package.new.json
mv package.new.json package.json
echo "[desktop deps: fetching]"
rm -rf .pnpm-store node_modules || true
pnpm i --frozen-lockfile

# make the proxy registry save what it recorded
kill -SIGINT $PID_RECORD
wait $PID_RECORD
cd -

# update build receipe
# ===========================================================

echo "[writing to manifest files]"
cat >generated/desktop-git.json <<EOL
[
    {
        "type": "git",
        "url": "https://github.com/deltachat/deltachat-desktop.git",
        "commit": "${DESKTOP_COMMIT_HASH}",
        "dest": "."
    }
]
EOL
        # "tag": "${DESKTOP_CHECKOUT}",

# pnpm
# ===========================================================

echo "[pnpm package to install pnpm]"
result=$(npm view pnpm@9.11.0 --json | jq "{url: .dist.tarball, integrity: .dist.integrity}")
# Use Python to decode the integrity hash and construct the manifest source item
python3 - <<EOL > generated/pnpm.json
import json
import sys
import base64

data = json.loads('''$result''')

if data.get("integrity", "").startswith("sha512-"):
    output = {
        "type": "archive",
        "url": data["url"],
        "sha512": base64.b64decode(data["integrity"].replace("sha512-", "")).hex(),
        "dest": "pnpm"
    }
    print(json.dumps(output, indent=2))
else:
    print("Input package has unexpected hash, expected sha512", file=sys.stderr)
    sys.exit(1)
EOL

echo "[strip unused versions from pnpm package indices and unify them into one file]"
node tool_process.mjs
rm generated/used_versions_strip_info.json

echo "[done]"