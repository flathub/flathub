#!/bin/sh
# Extracts the application jar from the upstream deb in /app/extra.
# The Platform has no ar or dpkg, so the ar container is read by hand: an 8-byte
# magic, then per member a 60-byte header (name in bytes 1-16, size in bytes 49-58)
# and the data, padded to an even length.
set -e

deb=adalet-eimza-tray.deb
total=$(stat -c %s "$deb")
offset=8

while [ "$offset" -lt "$total" ]; do
    header=$(dd if="$deb" bs=1 skip="$offset" count=60 2>/dev/null)
    name=$(printf '%s' "$header" | cut -c1-16 | tr -d ' /')
    size=$(printf '%s' "$header" | cut -c49-58 | tr -d ' ')
    data=$((offset + 60))

    case "$name" in
        data.tar.xz)  flag=-J ;;
        data.tar.gz)  flag=-z ;;
        data.tar.zst) flag=--zstd ;;
        data.tar)     flag= ;;
        *)            flag=skip ;;
    esac

    if [ "$flag" != skip ]; then
        mkdir unpacked
        # shellcheck disable=SC2086
        tail -c +$((data + 1)) "$deb" | head -c "$size" |
            tar -x $flag --no-same-owner -C unpacked
        break
    fi

    offset=$((data + size + size % 2))
done

mv unpacked/usr/lib/adalet-eimza-tray/lib/app/eimza-tray-*.jar .
rm -rf unpacked "$deb"

ls eimza-tray-*.jar >/dev/null
