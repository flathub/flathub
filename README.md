# Flatpak packaging for Supersonic

[Supersonic](https://github.com/dweymouth/supersonic/) is a desktop music players that works with
Subsonic-compatible servers like [Navidrome](https://www.navidrome.org/about/).

To build this locally, try:

    flatpak-builder build-dir com.github.dweymouth.supersonic.yml

Then to install:

    flatpak-builder --user --install --force-clean build-dir com.github.dweymouth.supersonic.yml

And run:

    flatpak run com.github.dweymouth.supersonic

You will need `flatpak` and `flatpak-builder` packages for the above
to work.

Once this is [published on Flathub](https://discourse.flathub.org/t/supersonic-lightweight-cross-platform-desktop-client-for-subsonic-music-servers/3984/), only `flatpak` will be
required for running, of course.

To regenerate the `go-sources.json` file, head for the relevant
Supersonic source tree and run:

    go mod vendor
    flatpak-builder-tools/go-get/flatpak-go-vendor-generator.py vendor/modules.txt > golang_sources.json

Also copy the `vendor/modules.txt` file over.

You can find the latter script in [flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools/tree/master/go-get).
