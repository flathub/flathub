#!/bin/bash
export LD_LIBRARY_PATH=/app/extra/AutopanoPro
export QT_PLUGIN_PATH=/app/extra/AutopanoPro

/app/extra/AutopanoPro/AutopanoPro "$@"
