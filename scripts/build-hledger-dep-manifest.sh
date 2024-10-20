#!/usr/bin/env bash
set -Eeuo pipefail

# config
hledger_release=1.40

####

# dependencies
for program in cabal cabal-flatpak jq
do
    if ! command -v "$program" >/dev/null
    then echo "Required program '$program' not installed" && exit 1
    fi
done

####

dir=$(dirname "$(realpath "$0")")
dest="$dir/../hledger-dep.json"
flatpak_cabal_config="$dir/cabal-flatpak.json"
hledger_app_manifest=$(mktemp)

# Use cabal-fiatpak to generate an app manifest for hledger.
(
    cd "$(mktemp -d)"
    cabal get "hledger-$hledger_release"
    cd "hledger-$hledger_release"
    # Dry-run build to generate build plan (plan.json)
    cabal new-build --dry-run --disable-tests --disable-benchmarks
    # cabal-flatpak has a bug wrt. aarch64, so we add that ourselves in postprocess-hledger-manifest.jq
    cabal-flatpak --arch x86_64 "$flatpak_cabal_config" "$hledger_app_manifest"
)

# Process the hledger app manifest into a dependency manifest
jq -f $dir/postprocess-hledger-manifest.jq < "$hledger_app_manifest" > "$dest"
