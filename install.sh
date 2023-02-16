#!/bin/bash
mkdir /app/opt
cp -r waterfox/ /app/opt/waterfox
desktop-file-install --dir /app/share/applications net.waterfoxco.waterfox-g5.desktop
mkdir -p /app/share/icons/hicolor/256x256/apps/ /app/share/appdata
cp -p /app/opt/waterfox/browser/chrome/icons/default/default256.png /app/share/icons/hicolor/256x256/apps/net.waterfoxco.waterfox-g5.png
cp -p net.waterfoxco.waterfox-g5.appdata.xml /app/share/appdata
mkdir /app/bin
ln -sf /app/opt/waterfox/waterfox-bin /app/bin/waterfox
# Create prefrence file to block updates and 
mkdir -p /app/opt/waterfox/browser/defaults/preferences
echo "//This pref file disables checking of automatic updates" > /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo "//and checking the browser is the default, as flatpak" >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo "//doesn't play well with these features." >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo "" >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo 'pref("browser.shell.checkDefaultBrowser", false);' >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo 'pref("app.update.auto", false);' >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo 'pref("app.update.enabled", false);' >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
echo 'pref("app.update.autoInstallEnabled", false);' >> /app/opt/waterfox/browser/defaults/preferences/flatpak-prefs.js
# chmod +x /app/bin/waterfox