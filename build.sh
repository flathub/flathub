#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$project_dir"

for tool in flatpak flatpak-builder appstreamcli; do
    command -v "$tool" >/dev/null 2>&1 || {
        printf 'Commande requise : %s\n' "$tool" >&2
        exit 1
    }
done

builder_version=$(flatpak-builder --version)
builder_version=${builder_version##* }
builder_version=${builder_version##*-}
case "$builder_version" in
    0.*|1.0|1.0.*|1.1|1.1.*|1.2|1.2.*|1.3|1.3.*)
        printf '%s\n' 'flatpak-builder >= 1.4 est requis.' >&2
        exit 1
        ;;
esac

appstreamcli compose --help >/dev/null 2>&1 || {
    printf '%s\n' 'Le composant appstreamcli compose est requis.' >&2
    exit 1
}

if [ "$(flatpak --default-arch)" != x86_64 ]; then
    printf '%s\n' 'Cette archive NoNetChat 0.3.0 nécessite x86_64.' >&2
    exit 1
fi

for runtime in org.gnome.Platform//51 org.gnome.Sdk//51; do
    flatpak info "$runtime" >/dev/null 2>&1 || {
        printf 'Runtime requis : %s\n' "$runtime" >&2
        exit 1
    }
done

flatpak-builder --user --arch=x86_64 --default-branch=stable --disable-rofiles-fuse --force-clean --repo=repo \
    build-dir com.nonetchat.NoNetChat.yml
flatpak build-bundle --arch=x86_64 repo \
    com.nonetchat.NoNetChat-0.3.0-x86_64.flatpak com.nonetchat.NoNetChat stable
