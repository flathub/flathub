#!/bin/bash

##
## Create file "mailviewer-sources.json"
##

if [[ "$1" == "--deps" ]]; then
  if [[ ! -f flatpak-cargo-generator.py ]]; then
    wget https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/refs/heads/master/cargo/flatpak-cargo-generator.py
  fi

  if [[ ! -f Cargo.lock ]]; then
    wget https://raw.githubusercontent.com/alescdb/mailviewer/refs/heads/main/Cargo.lock
  fi

  if [[ ! -d .venv ]]; then
    python -m venv .venv
    .venv/bin/pip install aiohttp toml
  fi

  .venv/bin/python flatpak-cargo-generator.py \
    -o mailviewer-sources.json \
    Cargo.lock
fi

##
## Build flatpak
##
flatpak run org.flatpak.Builder \
  --force-clean \
  --sandbox \
  --user \
  --install \
  --install-deps-from=flathub \
  --ccache \
  --mirror-screenshots-url=https://dl.flathub.org/media/ \
  --repo=repo \
  builddir io.github.alescdb.mailviewer.json && {
  rm -rf /tmp/mailviewer
  RUST_LOG=mailviewer=debug flatpak run io.github.alescdb.mailviewer sample.eml
}
