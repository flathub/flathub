# SPDX-FileCopyrightText: © 2024 David Bliss
#
# SPDX-License-Identifier: GPL-3.0-or-later

# lint
lint:
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest ./app.fotema.Fotema.json
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo

# Locally test flatpak build for flathub
flathub:
    flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    flatpak run org.flatpak.Builder \
        --force-clean \
        --sandbox \
        --user \
        --install \
        --install-deps-from=flathub \
        --ccache \
        --mirror-screenshots-url=https://dl.flathub.org/media/ \
        --repo=repo \
        builddir \
        app.fotema.Fotema.json

