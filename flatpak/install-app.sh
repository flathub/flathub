#!/usr/bin/env bash
set -euo pipefail

install -d /app/mysoftphone
cp -r build/linux/x64/release/bundle/* /app/mysoftphone/
install -Dm755 FlatpakMySoftphone.sh /app/bin/FlatpakMySoftphone.sh
install -Dm644 ai.connectiva.mysoftphonepremium.desktop /app/share/applications/ai.connectiva.mysoftphonepremium.desktop
install -Dm644 flatpak/icons/ai.connectiva.mysoftphonepremium.png /app/share/icons/hicolor/512x512/apps/ai.connectiva.mysoftphonepremium.png
install -Dm644 ai.connectiva.mysoftphonepremium.metainfo.xml /app/share/metainfo/ai.connectiva.mysoftphonepremium.metainfo.xml

# Compatibility symlink for legacy SONAME
if [ -f /app/lib/libayatana-appindicator3.so.1 ] && [ ! -f /app/lib/libappindicator3.so.1 ]; then
  ln -sf /app/lib/libayatana-appindicator3.so.1 /app/lib/libappindicator3.so.1
fi