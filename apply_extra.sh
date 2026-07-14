#!/bin/bash
set -euo pipefail

tar -xzf Lisek-linux.tar.gz
# electron-builder packs as lisek-<version>/
mv lisek-* lisek
rm -f Lisek-linux.tar.gz
