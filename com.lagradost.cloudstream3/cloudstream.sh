#!/bin/sh
rm -rf "$XDG_CACHE_HOME/art"
export ATL_UGLY_ENABLE_WEBVIEW=
exec android-translation-layer --gapplication-app-id=com.lagradost.cloudstream3 /app/share/CloudStream.apk $@
