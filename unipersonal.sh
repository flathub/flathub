#!/bin/sh
set -e

# /app/unipersonal is only used by the local test manifest, which cannot
# populate /app/extra.
APPDIR=/app/extra
[ -d "$APPDIR" ] || APPDIR=/app/unipersonal

export LD_LIBRARY_PATH="$APPDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# The clipboard watcher normally starts from /etc/xdg/autostart, which a
# flatpak cannot write to. It takes an exclusive lock and exits if another
# instance is already running.
if [ -x "$APPDIR/unipersonal-clipboard" ]; then
    "$APPDIR/unipersonal-clipboard" >/dev/null 2>&1 &
fi

cd "$APPDIR"
exec "$APPDIR/UniPersonal.Desktop" "$@"
