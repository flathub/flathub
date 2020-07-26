## Test

1. Build and install the application locally:

      flatpak-builder --force-clean --install --user build-dir com.bitstower.Markets.json
1. Run newly intalled application:

      flatpak run com.bitstower.Markets

##  Notes

The _org.gnome.Platform_ runtime provides the following libraries:

* libsoup
* libgee
* json-glib
* glib2
* gtk3

...which are required by the application.