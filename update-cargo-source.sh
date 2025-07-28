#!/bin/bash

set -euo pipefail
set -x

base_dir="$(dirname "${BASH_SOURCE[0]}" | xargs realpath)"

REPO="heathcliff26/turbo-clicker"
BASE_URL="https://raw.githubusercontent.com/${REPO}/refs/tags/"
VENV_PATH="venv"

tag="$(yq -r .modules[0].sources[0].tag io.github.heathcliff26.turbo-clicker.yaml)"

cargo_lock_url="${BASE_URL}${tag}/Cargo.lock"

curl -SLO "${cargo_lock_url}"

pushd "${base_dir}"

[ ! -e flatpak-builder-tools ] && git clone --depth 1 --branch master https://github.com/flatpak/flatpak-builder-tools.git

pushd flatpak-builder-tools/cargo

[ ! -e venv ] && python3 -m venv "${VENV_PATH}"
# shellcheck disable=SC1091
source venv/bin/activate
pip install -U pip setuptools
pip install poetry
poetry install
poetry env activate
python3 flatpak-cargo-generator.py ../../Cargo.lock -o ../../cargo-sources.json

popd
popd
