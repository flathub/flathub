#!/bin/sh
set -eu

# Les PNG proviennent de l'icône d'origine du bundle vérifié.
# Usage : sh icons/install-icons.sh RACINE_HICOLOR
source_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
target_root=${1:?Repertoire hicolor requis}

for size in 16 24 32 48 64 128 256 512; do
    relative="${size}x${size}/apps/com.nonetchat.NoNetChat.png"
    install -Dm644 "$source_root/$relative" "$target_root/$relative"
done
