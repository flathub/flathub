#!/usr/bin/env sh

set -e

MANIFEST="io.github.TeamWheelWizard.WheelWizard.yaml"

BUILDER_APP_ID="org.flatpak.Builder"

cleanup() {
    rm -rf .flatpak-builder
    rm -rf build-dir
    rm -rf repo
}

flatpak remote-add --user --if-not-exists flathub 'https://dl.flathub.org/repo/flathub.flatpakrepo'
flatpak install --user --noninteractive flathub "$BUILDER_APP_ID"

cleanup

flatpak run --user "$BUILDER_APP_ID" \
    --user \
    --install-deps-from=flathub \
    --install-deps-only \
    build-dir \
    "$MANIFEST"

cleanup

flatpak run --user "$BUILDER_APP_ID" \
    --user \
    --force-clean \
    --install \
    --repo=repo \
    --disable-rofiles-fuse \
    build-dir \
    "$MANIFEST"

cleanup
