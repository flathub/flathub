#!/bin/sh

if [ -z $NVIDIA_VERSION ]; then
	export NVIDIA_VERSION=$1
fi

if [ -z $REPO_LOCATION ]; then
	export REPO_LOCATION="nvidia-settings"
fi
sed -e "s/@@NVIDIA_VERSION@@/$NVIDIA_VERSION/g" < nvidia-settings.json.in > nvidia-settings.json

flatpak-builder com.nvidia.nvidia-settings nvidia-settings.json --repo=$REPO_LOCATION --force-clean
