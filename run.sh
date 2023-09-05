#!/bin/sh
exec /app/jre/bin/java -Dawt.useSystemAAFontSettings=on -Dswing.aatext=true -jar /app/bin/corese-gui.jar "$@"
