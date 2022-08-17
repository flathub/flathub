# Boram Flatpak

An unnoficial Flathub package for Boram.

## Local build and install

```sh
# Build
flatpak run org.flatpak.Builder build-dir com.github.kagami.boram.yml --force-clean

# Install
flatpak run org.flatpak.Builder --user --install --force-clean build-dir com.github.kagami.boram.yml 
```