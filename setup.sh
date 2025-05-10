#!/usr/bin/env bash
set -e

if which "pnpm" > /dev/null 2>&1; then
    echo "pnpm is already installed."
else
    echo "pnpm is not installed, trying to install it"
    npm i -g pnpm   
fi

git submodule init

mkdir working_dir || true
cd working_dir
git clone https://github.com/deltachat/deltachat-desktop.git

python3 -m venv .venv
source .venv/bin/activate
pip install aiohttp toml

git clone https://github.com/flatpak/flatpak-builder-tools/ --depth 1
pip install pipx
pipx install flatpak-builder-tools/node