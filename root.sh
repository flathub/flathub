#!/bin/sh

id="io.github.niccokunzmann.python_dhcp_server"

set -e
cd "`dirname \"$0\"`"

echo "Uninstall the currently installed application"
flatpak uninstall -y $id || true

sudo -i flatpak install -y "`pwd`/$id.flatpak"
sudo -i flatpak run "$id"
