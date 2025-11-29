#!/bin/bash
set -e

COMMIT_HASH="6d09d72df0ba5f8817a29d6edc265b911d2db0f2"

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
