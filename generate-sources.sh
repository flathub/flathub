#!/usr/bin/env bash

git clone https://github.com/acristoffers/Lachesis
git clone https://github.com/flatpak/flatpak-builder-tools

pushd Lachesis || exit
git checkout "$(yq -r '.modules[0].sources[0].commit' ../me.acristoffers.Lachesis.yml)"
popd || exit

pushd flatpak-builder-tools/node || exit

poetry install --no-root

poetry run flatpak-node-generator -o out1.json yarn ../../Lachesis/yarn.lock
poetry run flatpak-node-generator -o out2.json yarn ../../Lachesis/Lachesis/yarn.lock
poetry run flatpak-node-generator -o out3.json yarn ../../Lachesis/desktop/yarn.lock
poetry run flatpak-node-generator -o out4.json yarn ../../Lachesis/src/desktop/yarn.lock

jq -sc flatten out1.json out2.json out3.json out4.json > ../../tmp.json

popd || exit

jq -c unique tmp.json > tmp2.json
jq -c '[.[] | select((has("only-arches") | not) or (."only-arches" | contains(["x86_64"])))] | sort_by(.type)' tmp2.json > generated-sources.json

rm -rf Lachesis flatpak-builder-tools tmp{,2}.json
