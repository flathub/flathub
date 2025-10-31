#!/bin/bash
export TMPDIR=$XDG_CACHE_HOME/tmp
exec /app/glide/glide --name app.glide_browser.glide "$@"
