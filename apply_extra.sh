#!/usr/bin/env bash
set -e

chmod +x squidservers.AppImage
unappimage squidservers.AppImage > /dev/null
rm -f squidservers.AppImage
mv squashfs-root/* ./
rm -rf squashfs-root
