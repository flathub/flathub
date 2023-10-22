#!/bin/sh

ABEROTH="$XDG_DATA_HOME/aberoth/Aberoth.jar"

[[ ! -f "$ABEROTH" ]] \
    && install -Dm755 /app/extra/Aberoth.jar "$ABEROTH"
exec java -jar "$ABEROTH" "$@"
