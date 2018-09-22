#!/bin/bash

sizes=(8x8 16x16 22x22 24x24 32x32 48x48 64x64 96x96 256x256 512x512)

DIR=$(dirname "$(readlink -f "$0")")
cd "$DIR/../files"

for item in "${sizes[@]}"
do
	suffix=$(echo "$item" | cut -d x -f1)
	convert Adobe_Reader_v9.0_icon.png  -resize "$item" "icons/AdobeReader9-$suffix.png"
done
