#!/bin/bash

ARGS=()

while (($#)); do
    case "$1" in
        --prefix=*)
            echo "ignoring --prefix= arg" >&2
        ;;
        --libdir=*)
            echo "ignoring --libdir= arg" >&2
        ;;
        *)
            ARGS+=("$1")
        ;;
    esac
    shift
done

echo "winemaker args: ${ARGS[*]}" >&2

exec winemaker "${ARGS[@]}"
