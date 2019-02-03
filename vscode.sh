#!/usr/bin/bash

flatpak='flatpak-spawn --host flatpak'

vscode_title='Visual Studio Code'
vscode_oss_title='Visual Studio Code - OSS'

if [[ `basename $0` == vscode-oss-editor ]]; then
  ref=com.visualstudio.code.oss
  title=$vscode_oss_title
  other_title=$vscode_title
  other_basename=vscode
else
  ref=com.visualstudio.code
  title=$vscode_title
  other_title=$vscode_oss_title
  other_basename=vscode-oss
fi

if $flatpak info -m $ref >/dev/null 2>&1; then
  exec $flatpak run $ref "$@"
else
  zenity --error --no-wrap --title="$title is not installed" \
  --text="The $title Flatpak has not been installed, please install it from Flathub.\n\
If you meant to use $other_title, set the editor to /app/bin/$other_basename-editor instead."
fi

