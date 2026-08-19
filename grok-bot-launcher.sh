#!/usr/bin/env bash
set -euo pipefail
export TMPDIR="${XDG_RUNTIME_DIR:-/tmp}"
exec /app/grok-bot/grok-bot --no-sandbox --disable-gpu-sandbox "$@"
