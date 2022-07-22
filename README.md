## Flatpak documentation

If you want to build the flatpak for TLPUI yourself please follow the steps below:

1. Make sure you have the Flathub repository installed like this:

  `flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo`

2. To build and install the Flatpak in user environment you can call from inside the **flatpak** folder:

  `flatpak-builder --force-clean --user --install-deps-from flathub --install build-dir com.github.d4nj1.tlpui.yml`

3. To run the Flatpak execute:

  `flatpak run com.github.d4nj1.tlpui`
