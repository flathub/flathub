#!/usr/bin/env bash

exec java -Dsun.java2d.xrender=f -Xmx256m -Xms128m -jar /app/share/es.estoes.wallpaperDownloader/wallpaperdownloader.jar "$@"
