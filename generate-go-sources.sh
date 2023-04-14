#!/bin/sh

# heavily modified from com.yktoo.ymuse:
# https://github.com/flathub/com.yktoo.ymuse/blob/7c33add48cfc6788b9ddf0c92ee7213f40c6c78e/com.yktoo.ymuse.yml#L56-L63
set -e

go mod download -json |
    jq -r .Zip |
    sed -E 's|.*?download/|https://proxy.golang.org/|' |
    while read u; do
        echo >&2 checksumming $u...
        sha256=$(curl -f "$u" | sha256sum -b | cut -d ' ' -f 1)
        cat <<EOF
- type: archive
  url: $u
  strip-components: 0
  dest: vendor
  sha256: $sha256

EOF
    done
