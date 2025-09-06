#!/usr/bin/env bash

set -euo pipefail

APP_DIR="/app"
WORK_DIR="${XDG_CACHE_HOME:-/var/tmp}/orange3"
VENV_DIR="${WORK_DIR}/.venv"

mkdir -p "${WORK_DIR}"
cp -f "${APP_DIR}/pyproject.toml" "${WORK_DIR}/pyproject.toml"
cp -f "${APP_DIR}/uv.lock" "${WORK_DIR}/uv.lock"

cd "${WORK_DIR}"

# Reuse existing venv if present, otherwise create it explicitly at VENV_DIR
if [ ! -x "${VENV_DIR}/bin/python" ]; then
  "${APP_DIR}/bin/uv" venv --system-site-packages "${VENV_DIR}"
fi

"${APP_DIR}/bin/uv" run --no-group dev --locked python -m Orange.canvas
