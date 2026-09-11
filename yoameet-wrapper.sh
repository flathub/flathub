#!/bin/sh
# Lance le binaire Flutter en préservant lib/ et data/ relatifs.
set -eu
APPDIR="/app/yoa"
# /app/lib + /app/lib64 : deps Flatpak (libsecret, etc.)
# ${APPDIR}/lib : plugins Flutter (.so du bundle)
export LD_LIBRARY_PATH="/app/lib:/app/lib64:${APPDIR}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "${APPDIR}"
exec "${APPDIR}/yoa" "$@"
