# Elk

## Building

You can build and directly install the built Flatpak:

    flatpak-builder --install ./build ./zone.elk.Elk.yml --force-clean -y

or export the Flatpak into a repo for later installation or bundling:

    flatpak-builder --repo ./repo ./build ./zone.elk.Elk.yml --force-clean

Install from repository:

    flatpak install ./repo zone.elk.Elk

Export bundle:

    flatpak build-bundle ./repo ./Elk.flatpak zone.elk.Elk


