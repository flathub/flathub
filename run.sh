#!/usr/bin/env bash

set -ex

java \
  -Djava.util.prefs.userRoot=$XDG_CONFIG_HOME \
  -Djava.io.tmpdir=$XDG_CACHE_HOME/tmp/ \
  -jar /app/bin/tambourine.jar