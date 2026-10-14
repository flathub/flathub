#!/bin/bash
export TMPDIR=${XDG_CACHE_HOME:-${HOME}/.cache}
exec /app/opt/louvorja-piano/louvorja-piano --no-sandbox "$@"