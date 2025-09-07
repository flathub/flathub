#!/usr/bin/env bash

set -euo pipefail

APP_DIR="/app"
WORK_DIR="${XDG_CACHE_HOME:-/var/tmp}/orange3"
VENV_DIR="${WORK_DIR}/.venv"

mkdir -p "${WORK_DIR}"
cd "${WORK_DIR}"

if ! test -f ./pyproject.toml; then
  uv init --bare
fi

# Reuse existing venv if present, otherwise create it explicitly at VENV_DIR
if [ ! -x "${VENV_DIR}/bin/python" ]; then
  uv venv --system-site-packages "${VENV_DIR}"
fi

uv add pip
uv run python -m Orange.canvas
