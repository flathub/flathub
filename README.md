# Zrythm Flatpak

Flatpak build of [Zrythm](https://www.zrythm.org), a highly automated and intuitive digital audio workstation.

## Plugins

This package supports Flatpak builds of LV2, LXVST, and VST3 plugins. Plugins not installed as Flatpak cannot be used.

You can install Plugins using GNOME Software. Or, to view a list of installable plugins, run:  
`flatpak install org.freedesktop.LinuxAudio.Plugins//21.08 -y`

## JACK Support

You need to install `pipewire-jack-audio-connection-kit` (included in Fedora) or `pipewire-jack` (Arch) which replaces JACK with the PipeWire implementation of JACK. Using normal JACK, without PipeWire, is not supported.

## Memory Locking
At runtime you might be warned about memory locking. Memory locking priviliges are useful for reliable, dropout-free operation. This is not specific to Flatpak.

You can follow [the guide listed in the Zrythm docs](https://manual.zrythm.org/en/getting-started/system-requirements.html#gnu-linux).

## Maintainer notes:

Intended for Zrythm maintainers or experienced users.

Build:

Prerequisites:  
Assuming you have Flathub installed, along with `flatpak` and `flatpak-builder`.
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

1. Clone the repo
```
git clone --recurse-submodules https://github.com/vchernin/flathub --branch org.zrythm.Zrythm
cd flathub
```

2. Build the Flatpak
```
flatpak-builder --repo=zrythm --force-clean --install-deps-from=flathub --ccache --user build-dir org.zrythm.Zrythm.json
```

3. Add the local repo and install the Flatpak
```
flatpak remote-add --user zrythm zrythm --no-gpg-verify
flatpak install --user zrythm org.zrythm.Zrythm
```
4. Run the Flatpak build 
```
flatpak run org.zrythm.Zrythm
```

Optionally after steps 1-3 you can build and install [a bundle](https://docs.flatpak.org/en/latest/single-file-bundles.html) with:
```
flatpak build-bundle zrythm zrythm.flatpak org.zrythm.Zrythm
flatpak install zrythm.flatpak
```

Build-time issues:

When building with ccache, sometimes vamp-plugin-sdk complains about the object file, rm .flatpak-builder and build-dir (i.e. reset the cache) to fix it.

When upstream uses native PipeWire API (not PipeWire-JACK) will need:
- ensure settings "manager" permission https://gitlab.freedesktop.org/pipewire/pipewire/-/issues/667#note_787310
