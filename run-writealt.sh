#!/bin/sh
export TMPDIR="${XDG_CACHE_HOME:-${HOME}/.cache}"
exec zypak-wrapper /app/extra/writealt "$@"
