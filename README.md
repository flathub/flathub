# Thonny Flatpak

This is the official [Thonny](https://thonny.org/) Flatpak.

## Users

This section covers installation instructions and additional information for Thonny users.

### Install

Add the Flathub remote.

    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

Install the Thonny Flatpak.

    flatpak install flathub org.thonny.Thonny

## Maintainers

### Build

Get the source code.

    git clone https://github.com/flathub/org.thonny.Thonny.git
    cd org.thonny.Thonny/packaging/linux

Add the Flathub repository.

    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

Install the FreeDesktop SDK and Platform.

    flatpak install --user flathub org.freedesktop.Sdk//20.08

Install Flatpak Builder.

    sudo apt install flatpak-builder

Build the Flatpak.

    flatpak-builder --user --install --force-clean --repo=repo thonny-flatpak-build-dir org.thonny.Thonny.yaml

Run the Flatpak.

    flatpak run org.thonny.Thonny

### Update

The Python dependencies for the Flatpak are generated with the help of the [Flatpak Pip Generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/pip).
This tool produces `json` files for Python packages to be included in the Flatpak manifest's `modules` section.
In order to update or add dependencies in the Flatpak, these dependencies can be generated using the following instructions.

First, install the Python dependency `requirements-parser`.

    python3 -m pip install requirements-parser

Now run the Flatpak Pip Generator script for the necessary packages.
The necessary packages are listed in the files `packaging/requirements-regular-bundle.txt` and `packaging/requirements-xxl-bundle.txt` in Thonny's repository.
The following command shows how to retrieve packages from Thonny's `requirements.txt` file by producing a `python3-modules.json` file.
I usually convert these to YAML and place them directly in the Flatpak manifest for readability.

    wget -L https://raw.githubusercontent.com/thonny/thonny/master/requirements.txt
    python3 flatpak-builder-tools/pip/flatpak-pip-generator --runtime org.freedesktop.Sdk//20.08 $(cat requirements.txt)

If you have `org.freedesktop.Sdk//20.08` installed in *both* the user and system installations, the Flatpak Pip Generator will choke generating the manifest.
The best option at the moment is to temporarily remove either the user or the system installation until this issue is fixed upstream.
