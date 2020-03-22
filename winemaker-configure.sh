#!/bin/bash

ARGS=()

while (($#)); do
    case "$1" in
        --prefix=*|--libdir=*)
            echo "$0: ignoring arg: $1" >&2
        ;;
        *)
            ARGS+=("$1")
        ;;
    esac
    shift
done

echo "$0: winemaker args: ${ARGS[*]}" >&2

exec winemaker "${ARGS[@]}"
