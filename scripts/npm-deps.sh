#!/bin/bash

# pip install 
git clone https://github.com/flatpak/flatpak-builder-tools
cd flatpak-builder-tools/node
pip install .
cd ../../
rm -rf flatpak-builder-tools

python3 -m flatpak_node_generator -o "$1" npm "$2"