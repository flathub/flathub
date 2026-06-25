#!/bin/sh
# Exécuté à l'installation (CWD = dossier extra contenant voxcut.tar.gz).
# Décompresse le bundle PyInstaller puis supprime l'archive.
set -e
tar -xf voxcut.tar.gz
rm -f voxcut.tar.gz
