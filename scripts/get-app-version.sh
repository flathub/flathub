#!/bin/bash

yq '
  .modules[]
  | select(.name == "no_more_background")
  | .sources[]
  | select(.url == "*no_more_background.git")
  | .tag
' flatpak-flutter.yaml
exit $?
