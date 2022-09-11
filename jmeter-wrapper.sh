#!/bin/sh

JMETER_CONFIG="${XDG_CONFIG_HOME}/jmeter/user.properties"
JMETER_CLASSPATH="${XDG_DATA_HOME}/jmeter/lib"
JMETER_LIBEXT="${JMETER_CLASSPATH}/ext"
JMETER_LOGFILE="${XDG_CACHE_HOME}/jmeter/jmeter.log"

mkdir -p "$JMETER_CLASSPATH" "$JMETER_LIBEXT" \
  "${XDG_CONFIG_HOME}/jmeter" \
  "${XDG_CACHE_HOME}/jmeter"

[ -r "$JMETER_CONFIG" ] || cp /app/bin/user.properties "$JMETER_CONFIG"

# Force help.local=true since browser integration is not working
# https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk17/issues/1

exec /app/bin/jmeter \
  -J "help.local=true" \
  -J "search_paths=${JMETER_LIBEXT}" \
  -J "user.classpath=${JMETER_CLASSPATH}" \
  -j "$JMETER_LOGFILE" \
  -p "${JMETER_CONFIG}" \
  $@
