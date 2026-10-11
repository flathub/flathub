#!/bin/sh
set -e
# Offset du squashfs = fin de l'en-tête ELF de l'AppImage (--appimage-extract ne marche pas dans le bac à sable)
rd() { od -An -t u"$2" -j "$1" -N "$2" ledger-live-desktop.AppImage | tr -d ' '; }
shoff=$(rd 40 8); shentsize=$(rd 58 2); shnum=$(rd 60 2)
offset=$((shoff + shentsize * shnum))
unsquashfs -no-progress -o "$offset" -d ledger-live ledger-live-desktop.AppImage >/dev/null
rm -rf ledger-live/AppRun ledger-live/usr ledger-live/chrome-sandbox ledger-live/*.desktop
rm -f ledger-live-desktop.AppImage
