# Flatpak for Midori Web Browser

Assuming [Flatpak is correctly installed](https://flatpak.org/setup) ensure the runtime and SDK are installed:

    flatpak install flathub org.gnome.Platform//3.28 org.gnome.Sdk//3.28

Build using [flatpak-builder](http://docs.flatpak.org/en/latest/flatpak-builder.html):

    flatpak-builder _build org.midori_browser.Midori.json --force-clean

Test the locally built package:

    flatpak-builder --run _build org.midori_browser.Midori.json midori
