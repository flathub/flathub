#!/bin/bash

flatpak=( $(curl -s https://packages.microsoft.com/repos/edge/dists/stable/main/binary-amd64/Packages.gz | gzip -d | grep -Em3 '^(SHA256|Filename|Size): ' | sort | cut -d' ' -f2) )

echo "url: https://packages.microsoft.com/repos/edge/${flatpak[0]}"
echo "sha256: ${flatpak[1]}"
echo "size: ${flatpak[2]}"
