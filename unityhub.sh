#!/usr/bin/bash

if [ ! -f /var/data/eula-accept ]; then
  zenity --text-info --title="Unity Hub" --filename=/app/extra/eula.txt --ok-label=Agree --cancel-label=Disagree || exit 1
  touch /var/data/eula-accept
fi

if [ ! -f $XDG_DATA_HOME/unity3d/prefs ]; then
  b64editor=$(echo -n /app/bin/vscode-editor | base64)
  b64args=$(echo -n '"$(File)"' | base64)
  mkdir -p $XDG_DATA_HOME/unity3d
  cat >$XDG_DATA_HOME/unity3d/prefs <<EOF
<unity_prefs version_major="1" version_minor="1">
  <pref name="kScriptEditorArgs" type="string">$b64args</pref>
  <pref name="kScriptEditorArgs/app/bin/vscode-editor" type="string">$b64args</pref>
  <pref name="kScriptsDefaultApp" type="string">$b64editor</pref>
</unity_prefs>
EOF
fi

UNITY_DATADIR=$XDG_DATA_HOME TMPDIR=$XDG_CACHE_HOME /app/extra/unityhub "$@"
