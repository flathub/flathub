#!/usr/bin/env bash

set -ex

XDG_DATA_HOME=${XDG_DATA_HOME:-~/.var/app/com.todesk.ToDesk/data}
APP_HOME="$XDG_DATA_HOME/app"

mkdir -p "$APP_HOME/todesk"
cur_uuid=$(cat /app/extra/uuid)

if test ! -f "$APP_HOME/uuid" -o "$(cat $APP_HOME/uuid)" != "$cur_uuid"; then
    rm -rf "$APP_HOME/todesk/bin" "$APP_HOME/todesk/res"
    cp -a /app/extra/todesk/bin "$APP_HOME/todesk/"
    cp -a /app/extra/todesk/res "$APP_HOME/todesk/"
    cp -a /app/extra/uuid "$APP_HOME/uuid"
fi

export TODESK_PACK_NAME=todesk
export LIBVA_DRIVER_NAME=iHD
export LIBVA_DRIVERS_PATH="$APP_HOME/todesk/bin"

"$APP_HOME/todesk/bin/ToDesk_Service" &
exec "$APP_HOME/todesk/bin/ToDesk"

