#!/bin/sh

# workaround for https://github.com/eclipse-platform/eclipse.platform.swt/issues/568
if [ ! -f ~/.swt/trims.prefs ]; then
    mkdir -p ~/.swt
    cat > ~/.swt/trims.prefs <<_END
trimWidths=0 4 6 0 6 0
trimHeights=0 4 6 0 29 23
_END
fi

exec /app/jre/bin/java -jar /app/bin/nxmc.jar "$@"
