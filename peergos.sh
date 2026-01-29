#!/bin/sh

JAVA_HOME=/usr/lib/sdk/openjdk21
export PATH="$JAVA_HOME/bin:$PATH"

java -jar /app/share/peergos/Peergos.jar -flatpak true
