#!/bin/bash

PKG_JSON="main/package.json"

NAME=$(jq -r '.name' "$PKG_JSON")
EXEC=$(jq -r '.name' "$PKG_JSON")
ICON=$(jq -r '.build.linux.icon' "$PKG_JSON")
CATEGORY=$(jq -r '.build.linux.category' "$PKG_JSON")
DESCRIPTION=$(jq -r '.build.linux.description' "$PKG_JSON")
SYNOPSIS=$(jq -r '.build.linux.synopsis' "$PKG_JSON")

cat <<EOF > "${NAME}.desktop"
[Desktop Entry]
Name=${NAME}
Exec=${EXEC}
Icon=${ICON}
Type=Application
Categories=${CATEGORY}
Comment=${DESCRIPTION}
GenericName=${SYNOPSIS}
EOF