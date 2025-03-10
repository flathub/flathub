#!/bin/bash

# pip install 
# This is a temporary workaround for https://github.com/flatpak/flatpak-builder-tools/issues/445
# once https://github.com/flatpak/flatpak-builder-tools/pull/446 is merged, replace the below line with
# git clone https://github.com/flatpak/flatpak-builder-tools
git clone https://github.com/MoralCode/flatpak-builder-tools --single-branch --branch handle-spaces
cd flatpak-builder-tools/node
pip install .
cd ../../

python3 -m flatpak_node_generator -o "$1" npm "$2"

pip uninstall flatpak_node_generator -y

rm -rf flatpak-builder-tools
