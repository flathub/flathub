#!/bin/bash
export TMPDIR=${XDG_CACHE_HOME:-${HOME}/.cache}
exec /app/opt/Praxis/praxis-app --no-sandbox "$@"
