#!/bin/bash

set -euo pipefail

# THIS IS FOR TESTING PURPOSES ONLY. DO NOT USE THIS SCRIPT IN PRODUCTION.


# Helper functions to print messages, handle errors, and check for required commands
die() {
    echo "Error: $*" >&2
    exit 1
}

# Helper function to check for required commands, and exit with an error if not found
require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

require_cmd appstreamcli
require_cmd flatpak
require_cmd xmllint

echo ">>>>>>> Setup flathub remote if it doesn't exist"

flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo

echo ">>>>>>> Install org.flatpak.Builder if it wasn't already installed"
if ! flatpak list --user | grep -q org.flatpak.Builder; then
  flatpak install --user -y flathub org.flatpak.Builder
fi

echo ">>>>>>> xml-lint the share/metainfo/com.dygma.bazecor.metainfo.xml file to ensure that it is valid"
xmllint --noout share/metainfo/com.dygma.bazecor.metainfo.xml

echo ">>>>>>> Validate the share/metainfo/com.dygma.bazecor.metainfo.xml file to ensure that it is valid and meets the AppStream specification"
appstreamcli validate share/metainfo/com.dygma.bazecor.metainfo.xml

echo ">>>>>>> Run the com.dygma.bazecor.yml manifest through the linter to ensure that it is valid and meets the Flatpak specification"
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.dygma.bazecor.yml

echo ">>>>>>> Building the Bazecor Flatpak using the com.dygma.bazecor.yml manifest..."
flatpak run --command=flathub-build org.flatpak.Builder com.dygma.bazecor.yml

echo ">>>>>>> Installing the built flatpak to the user account, forcing a clean install to ensure that the new version is installed..."
flatpak install --user -y ./repo com.dygma.bazecor

echo ">>>>>>> Successfully built and installed Bazecor Flatpak!"
echo ">>>>>>> You can run it with:"
echo ">>>>>>>"
echo ">>>>>>>    flatpak run com.dygma.bazecor"
echo ">>>>>>>"
