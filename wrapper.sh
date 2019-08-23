#!/usr/bin/env bash

test -z "${STEAM_COMPAT_DATA_PATH}" && exit 1
export WINEPREFIX="${STEAM_COMPAT_DATA_PATH}/pfx"

BASEDIR="$(dirname "${BASH_SOURCE[0]}")"

case $1 in
    run)
        shift
        "$BASEDIR/bin/wine" "$@"
    ;;
    waitforexitandrun)
        shift
        "$BASEDIR/bin/wineserver" -w
        "$BASEDIR/bin/wine" "$@"
    ;;
    getnativepath)
        shift
        "$BASEDIR/bin/winepath" "$@"
    ;;
    getcompatpath)
        shift
        "$BASEDIR/bin/winepath" -w "$@"
    ;;
    *)
        exit 1
    ;;
esac
