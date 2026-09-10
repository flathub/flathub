#!/bin/bash
set -euo pipefail
export ELECTRON_DISABLE_SANDBOX=1
export TMPDIR="${XDG_RUNTIME_DIR:-/tmp}"
exec /app/lib/dalfin/dalfin --no-sandbox --disable-dev-shm-usage "$@"
