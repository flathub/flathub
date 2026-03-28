#!/bin/sh

exec /app/jre/bin/java \
    --add-opens=java.base/java.lang=ALL-UNNAMED \
    --add-opens=java.base/java.util=ALL-UNNAMED \
    --add-opens=java.base/java.lang.reflect=ALL-UNNAMED \
    --add-opens=java.base/java.text=ALL-UNNAMED \
    --add-opens=java.desktop/java.awt.font=ALL-UNNAMED \
    -Djava.library.path=/app/extra \
    -Dsun.java2d.uiScale.enabled=true \
    -jar /app/extra/szafir_703.jar "$@"
