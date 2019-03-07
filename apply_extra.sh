#!/bin/bash
unzip xmind.zip fonts* plugins/org.xmind.cathy_3.7.8.201807240049.jar &>/dev/null
unzip plugins/org.xmind.cathy_3.7.8.201807240049.jar -d cathy &>/dev/null
for size in 16 32 48 64 128 256 ; do
    mkdir -p export/share/icons/hicolor/${size}x${size}/apps ||:
    cp cathy/icons/xmind.${size}.png export/share/icons/hicolor/${size}x${size}/apps/net.xmind.XMind8.png
done
rm -fr plugins cathy
mkdir export/share/applications
cp /app/net.xmind.XMind8.desktop export/share/applications
