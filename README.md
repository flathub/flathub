# Stretchly Flatpak

The instructions here describe how to build and update the [Stretchly](https://hovancik.net/stretchly/) Flatpak.

## Build

Get the source code.

    git clone --recursive https://github.com/flathub/net.hovancik.Stretchly.git
    cd net.hovancik.Stretchly

Add the Flathub repository.

    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

Install Flatpak Builder.

    sudo apt install flatpak-builder

Build the Flatpak.

    flatpak-builder --user --install --install-deps-from=flathub --force-clean --repo=repo build-dir net.hovancik.Stretchly.yaml

Run the Flatpak.

    flatpak run net.hovancik.Stretchly

## Update

A list of package source archives and checksums are generated for *all* dependencies.
Ideally, these dependencies will be updated whenever the `package-lock.json` file changes to keep the Flatpak version using the same packages.
The Flatpak Node Generator is used to generate this list of packages from the `package-lock.json` file, which is included directly in the Flatpak manifest.
To generate an updated version of this file, `generated-sources.json`, follow the instructions here.

Make sure to install npm.

    sudo apt install jq npm pipx python3-aiohttp

Install yq.

    pipx install yq

Install the Flatpak Node Generator Python utility with `pipx`.

    pipx install flatpak-builder-tools/node

Ensure the Flatpak Node Generator and yq are in your `PATH`.

    pipx ensurepath

Fetch the Stretchly source code.

    git clone https://github.com/hovancik/stretchly.git

Checkout the appropriate tag.

    git -C stretchly switch -d $(yq -r '.modules[1].sources[4].commit' net.hovancik.Stretchly.yaml)

Run the script against the `package-lock.json` file as shown here.

    flatpak-node-generator npm stretchly/package-lock.json --electron-node-headers --recursive
