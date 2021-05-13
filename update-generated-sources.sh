#/usr/bin/env bash

#
# This script updates the generated sources from the upstream nightly build recipe.
#

TAG=$1

if [ -z ${TAG} ]; then
	echo "Usage: ./update-generated-sources.sh v21.05"
	exit 1
fi

wget -O generated/corrosion-generated-sources.json https://invent.kde.org/plasma-mobile/angelfish/-/raw/${TAG}/flatpak/corrosion-generated-sources.json
wget -O generated/generated-sources.json https://invent.kde.org/plasma-mobile/angelfish/-/raw/${TAG}/flatpak/generated-sources.json

