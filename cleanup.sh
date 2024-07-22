#!/bin/bash

# pyside
rm -rfv ${FLATPAK_DEST}/bin/pyside6-*
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/scripts

# webengine baseapp cleanup
[ -r ${FLATPAK_DEST}/cleanup-BaseApp-QtWebEngine.sh ] &&
  ${FLATPAK_DEST}/cleanup-BaseApp-QtWebEngine.sh