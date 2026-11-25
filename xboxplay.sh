#!/bin/sh
# zypak comes from org.electronjs.Electron2.BaseApp and replaces the setuid sandbox helper.
export CHROME_DESKTOP="${CHROME_DESKTOP:-com.wastedpatriot.XboxPlay.desktop}"
exec zypak-wrapper /app/lib/xboxplay/node_modules/electron/dist/electron /app/lib/xboxplay "$@"
