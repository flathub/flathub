#!/usr/bin/env bash

set -o pipefail
set -e

MANIFEST_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
TOOLS_DIR="${TOOLS_DIR:-"$MANIFEST_DIR/tools"}"

MANIFEST_PATH="$MANIFEST_DIR/io.neovim.nvim.tool.lazygit.yaml"
MODULE_NAME=${MODULE_NAME:-"delta"}
GENERATED_SOURCES="$MODULE_NAME-generated-sources.json"

MODULE_OBJ=$(
    python3 -c 'import sys,json,yaml; json.dump(yaml.safe_load(sys.stdin), sys.stdout)' \
    < "$MANIFEST_PATH" | \
    jq -e \
        --arg MODULE_NAME "$MODULE_NAME" \
        '.modules | map(select(objects | .name==$MODULE_NAME)) | first'
)
SOURCE_OBJ=$(
    jq -e '.sources | map(select(objects | .type=="git")) | first' \
    <<<"$MODULE_OBJ"
)

SOURCE_URL="$(jq -r '.url' <<<"$SOURCE_OBJ")"
SOURCE_TAG="$(jq -r '.tag' <<<"$SOURCE_OBJ")"

CLONE_DIR="$(mktemp -d "${TMPDIR:-"/tmp"}/$MODULE_NAME.XXXXXX")"

git clone --depth=1 --branch="$SOURCE_TAG" "$SOURCE_URL" "$CLONE_DIR"

while read -r patch_path; do
    echo "Applying patch $patch_path" >> /dev/stderr
    patch -d "$CLONE_DIR" -p1 < "$MANIFEST_DIR/$patch_path"
done < <(
    jq -r -e '.sources[] | objects | select(.type=="patch") | .path // .paths[]' \
    <<<"$MODULE_OBJ"
)

if [ -z "${PYTHONPATH}" ]; then
    export PYTHONPATH="$TOOLS_DIR/cargo"
else
    export PYTHONPATH="$TOOLS_DIR/cargo:${PYTHONPATH}"
fi

python3 -m flatpak-cargo-generator \
    --output "$MANIFEST_DIR/$GENERATED_SOURCES" \
    "$CLONE_DIR/Cargo.lock"

if ! git -C "$MANIFEST_DIR" diff --exit-code -- "$GENERATED_SOURCES" >> /dev/null; then
    git -C "$MANIFEST_DIR" add "$GENERATED_SOURCES"
    git -C "$MANIFEST_DIR" commit -m "Update $GENERATED_SOURCES for $MODULE_NAME $SOURCE_TAG"
fi
