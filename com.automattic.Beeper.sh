#!/bin/bash

exec env TMPDIR="${XDG_CACHE_HOME}" \
         ELECTRON_OZONE_PLATFORM_HINT=auto \
         zypak-wrapper /app/extra/beepertexts "$@"
