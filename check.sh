#!/bin/bash

function find_elfs {
    local directory="$1"
    local arch="$2"
    find "${directory}" -type f -not -path '*/debug/*' -print0 | \
    xargs -0 -n 10 -P "${FLATPAK_BUILDER_N_JOBS:-"1"}" -- file --no-pad -F "" -0 | \
    awk -F"\0" "\$2 ~ \"ELF ${arch}\" { print \$1; }"
}

for a in 64 32; do
    case "${a}" in
        64)
            dir_suffix=""
            not_a=32
        ;;
        32)
            dir_suffix="${a}"
            not_a=64
        ;;
    esac
    mapfile -t wrong_arch_files < <(find_elfs "/app/lib${dir_suffix}" "${not_a}")
    if [ ${#wrong_arch_files[@]} -ne 0 ]; then
        echo "ERROR: ${not_a}-bit files found in ${a}-bit directory:"
        printf '\t%s\n' "${wrong_arch_files[@]}"
        exit 1
    fi
done
