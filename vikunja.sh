#!/usr/bin/env bash

exec ${FLATPAK_DEST}/app/bin/zypak-wrapper.sh ${FLATPAK_DEST}/app/bin/extra/vikunja-desktop "$@"
