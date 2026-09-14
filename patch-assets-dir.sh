#!/bin/sh
set -e
sed -i 's|^ASSETS_DIR=.*# __INSTALLER_PATCH__|ASSETS_DIR="/app/share/divbar/assets" # __INSTALLER_PATCH__|' divbar.sh
