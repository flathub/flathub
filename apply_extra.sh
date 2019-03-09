#!/usr/bin/bash

unappimage *.AppImage

mv squashfs-root/* .
rm -rf squashfs-root *.AppImage
patch-resources resources/app.asar
rm AppRun
