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

To regenerate the modules list, run this against your supersonic
source tree (e.g. `~/dist/supersonic` below):

    go run github.com/dennwc/flatpak-go-mod@latest ~/dist/supersonic
