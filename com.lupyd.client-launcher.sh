#!/bin/sh
export APP_DIR="/app"
export LD_LIBRARY_PATH="$APP_DIR/lib:$APP_DIR/lib64:${LD_LIBRARY_PATH}"
export CEF_PATH="$APP_DIR/lib"
export CHROME_DEVEL_SANDBOX=""
export PATH="$APP_DIR/bin:${PATH}"
export XDG_DATA_DIRS="$APP_DIR/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
exec "$APP_DIR/bin/app" --no-sandbox "$@"
