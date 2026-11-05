#!/usr/bin/env bash
set -euo pipefail

CUR_DIR="$PWD"
SOURCE_URL="https://github.com/Aster-Privacy/Aster-Mail"
COMMIT=$(python3 - << 'END'
import yaml

with open("org.astermail.Mail.yml") as file:
    manifest = yaml.safe_load(file)

module = next(
    m
    for m in manifest["modules"]
    if isinstance(m, dict) and m.get("name") == "aster-mail"
)

source = next(
    s for s in module["sources"] if isinstance(s, dict) and s.get("type") == "git"
)

print(source["commit"])
END
)


echo "Source commit is: $COMMIT"
echo "Setting up work environment"
WORKDIR="$(mktemp -d -t aster-mail-flatpak-XXXXXX)"
cd "$WORKDIR"
python -m venv venv
source ./venv/bin/activate
pip install pipx
# Using unofficial pypi distribution of the script to avoid setup using poetry
pip install flatpak-cargo-generator
pipx install git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node --force


echo "Cloning into the repository"
git clone $SOURCE_URL aster_mail
git -C aster_mail checkout "$COMMIT"
cd aster_mail


echo "Generating node sources"
flatpak-node-generator npm package-lock.json -o node-sources.json


echo "Generating cargo sources"
flatpak-cargo-generator src-tauri/Cargo.lock -o cargo-sources.json


echo "Copying sources from temporary directory to the repository"
cp ./node-sources.json "$CUR_DIR/node-sources.json"
cp ./cargo-sources.json "$CUR_DIR/cargo-sources.json"
echo "Done!"
