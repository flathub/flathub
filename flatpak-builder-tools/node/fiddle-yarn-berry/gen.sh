#!/bin/bash

dir="$(dirname $(realpath $0))"
commit=98c0605f804df86df8636a477515bbe178cd3943

pushd "$dir/.."
curl -L "https://github.com/electron/fiddle/raw/$commit/yarn.lock" > "$dir/yarn.lock"
poetry run flatpak-node-generator yarn -o "$dir/generated-sources.json" "$dir/yarn.lock"
popd
