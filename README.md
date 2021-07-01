# Insomnia for Flatpak

You need to have the SDKs locally:

    flatpak install org.freedesktop.Sdk//20.08
    flatpak install org.electronjs.Electron2.BaseApp//20.08

To build and install locally use:

    flatpak-builder --user --install build-dir rest.insomnia.client.yml --force-clean
