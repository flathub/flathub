#!/usr/bin/env bash

set -ex

# Create tmp directory if missing
mkdir -p "$XDG_CACHE_HOME/tmp/"

exec java \
  "-Djava.util.prefs.userRoot=$XDG_CONFIG_HOME" \
  "-Djava.io.tmpdir=$XDG_CACHE_HOME/tmp/" \
  -jar /app/bin/tambourine.jar