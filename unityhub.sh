#!/usr/bin/bash

if [ ! -f /var/data/eula-accept ]; then
  zenity --text-info --title="Unity Hub" --filename=/app/extra/eula.txt --ok-label=Agree --cancel-label=Disagree || exit 1
  touch /var/data/eula-accept
fi
UNITY_DATADIR=$XDG_DATA_HOME TMPDIR=$XDG_CACHE_HOME /app/extra/unityhub "$@"
