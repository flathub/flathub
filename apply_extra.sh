set -o errexit

bsdtar -xf laby.deb
tar -xf data.tar.xz
mkdir -p ./launcher
cp -r usr/lib/labymodlauncher/* ./launcher
