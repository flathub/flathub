#!/bin/bash

JARPATH=$(dirname "$0")

echo "Extra args: " -- "$@"

JAVA_ARGS=()
OTHER_ARGS=()

for arg in "$@"; do
    echo "$arg"
    if [[ "$arg" =~ -D.* ]]; then
        JAVA_ARGS+=("$arg")
    else
        OTHER_ARGS+=("$arg")
    fi
done

echo "Java args: ${JAVA_ARGS[@]}"
echo "Other args: ${OTHER_ARGS[@]}"

java "${JAVA_ARGS[@]}" -jar "$JARPATH/zatikon.jar" "${OTHER_ARGS}"

