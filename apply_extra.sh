#!/usr/bin/bash

unappimage *.AppImage
mv squashfs-root/* .
rm -rf squashfs-root *.AppImage
# asar is a binary archive format, so the replacement string *must* be the same length
# as the original.
patch-resources resources/app.asar
# sed -i -f - resources/app.asar <<'EOF'
# EOF
rm AppRun
mkdir -p export/share/applications
sed '/X-AppImage/d;s/Exec=AppRun/Exec=start-unityhub/;s/Icon=unityhub/Icon=com.unity.UnityHub/' \
  unityhub.desktop > export/share/applications/com.unity.UnityHub.desktop
install -Dm 644 unityhub.png export/share/icons/hicolor/48x48/apps/com.unity.UnityHub.png
