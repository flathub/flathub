#!/bin/bash -e

echo "Preparing"

APP_IMAGE="/app/extra/lunar-client-2.9.3.appimage"

# Allow image to execute
chmod +x $APP_IMAGE

# Extract image
unappimage $APP_IMAGE

# Install data
echo "Installing data"

DEST="/app/extra/lunarclient.d/"
mkdir $DEST
cp -r squashfs-root/* $DEST

ICON_DIR="/app/extra/export/share/icons/hicolor/"

mkdir -p $ICON_DIR
cp -r squashfs-root/usr/share/icons/hicolor/* $ICON_DIR

iconSizes=("16" "32" "48" "64" "128" "256")

for I in "${iconSizes[@]}"
do
	dir="$ICON_DIR/${I}x${I}/apps/"
	mv "$dir/lunarclient.png" "$dir/com.lunarclient.LunarClient.png"
done

# Clean up
echo "Cleaning up"

rm -rf squashfs-root/

echo "Done"
