#!/bin/bash
set -e

COMMIT_HASH="7704b4f70065a4aa1b5f3b627738d497f325a50d"

rm -rf generated
mkdir generated
cat >generated/deltatouch-git.json <<EOL
[
    {
        "type": "git",
        "url": "https://codeberg.org/lk108/deltatouch.git",
        "commit": "${COMMIT_HASH}"
    }
]
EOL

rm -rf deltatouch-shallow
git clone --depth 1 --recurse-submodules --shallow-submodules https://codeberg.org/lk108/deltatouch deltatouch-shallow

uvx --from flatpak_cargo_generator flatpak-cargo-generator deltatouch-shallow/libs/chatmail-core/Cargo.lock -o generated/sources-rust.json
