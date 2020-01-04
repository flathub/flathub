#!/bin/sh

mkdir $FLATPAK_DEST/firestorm
mkdir $FLATPAK_DEST/lib/i386-linux-gnu
tar -xvf viewer.tar.xz --strip 1 -C $FLATPAK_DEST/firestorm
install -Dm755 firestorm-viewer $FLATPAK_DEST/bin/
install -Dm644 $FLATPAK_DEST/firestorm/firestorm_icon.png $FLATPAK_DEST/share/icons/hicolor/512x512/apps/org.firestormviewer.FirestormViewer.png
install -Dm644 org.firestormviewer.FirestormViewer-symbolic.svg $FLATPAK_DEST/share/icons/hicolor/symbolic/apps/org.firestormviewer.FirestormViewer-symbolic.svg
install -Dm755 launch_url.sh $FLATPAK_DEST/firestorm/etc
install -Dm644 org.firestormviewer.FirestormViewer.desktop $FLATPAK_DEST/share/applications/org.firestormviewer.FirestormViewer.desktop
install -Dm644 org.firestormviewer.FirestormViewer.metainfo.xml $FLATPAK_DEST/share/metainfo/org.firestormviewer.FirestormViewer.metainfo.xml
