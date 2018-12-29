# TntConnect Flatpak Package

This is an attempt to package TntConnect as flatpack package.

## Building

To build the flatpak, run the following command:

	flatpak-builder --force-clean --ccache --repo=tntconnect-repo build-dir com.tntware.TntConnect.json

To run the created flatpak:

	flatpak-builder --run build-dir com.tntware.TntConnect.json tntconnect.sh
