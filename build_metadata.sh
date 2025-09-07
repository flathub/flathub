#!/usr/bin/env bash

set -ex

# pyqt app pip packages:
# ===============================
# build             1.2.2.post1
# Cython            3.1.3
# flit_core         3.12.0
# installer         0.7.0
# Mako              1.3.10.dev0
# Markdown          3.8.2
# MarkupSafe        3.0.2
# meson             1.5.2
# packaging         25.0
# pip               25.2
# ply               3.11
# pycairo           1.28.0
# Pygments          2.19.2
# PyGObject         3.50.1
# pyparsing         3.2.3
# pyproject_hooks   1.2.0
# PyQt-builder      1.18.2
# PyQt6             6.9.1
# PyQt6_sip         13.10.2
# PyQt6-WebEngine   6.9.0
# setuptools        80.9.0
# setuptools-scm    8.3.1
# sip               6.12.0
# six               1.17.0
# tomli             2.2.1
# typing_extensions 4.14.0
# wheel             0.46.1

# uv.uaml
if ! test -e "flatpak-cargo-generator.py"; then
  wget https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/refs/heads/master/cargo/flatpak-cargo-generator.py -O flatpak-cargo-generator.py
fi
if ! test -e "uv-git"; then
  (git clone https://github.com/astral-sh/uv uv-git && cd uv-git && git checkout 0.8.15)
fi
uv run python flatpak-cargo-generator.py --yaml -o uv.yaml uv-git/Cargo.lock

#uv export --group orange --no-hashes --no-annotate --fork-strategy=fewest > requirements.txt
uv run pip-compile --extra orange --no-annotate -o requirements.txt
uv run req2flatpak --requirements orange3==3.39.0 --requirements-file ./requirements.txt --yaml --target-platforms 312-x86_64 312-aarch64 --outfile python3-orange3.yaml
