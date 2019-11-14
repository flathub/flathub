# Purr Data packaged for Flatpak

This is a Flatpak for [Purr Data](https://www.purrdata.net/).

Purr Data is a popular fork of the [Pure Data](http://puredata.info/), an open
source visual programming language for multimedia.

## How to build

This Flatpak uses the standard
[flatpak-builder](docs.flatpak.org/en/latest/flatpak-builder-command-reference.html)
tool to build.

You can run a command like the following to build the package from this repo
and install it in your 'user' Flatpak installation:

    flatpak-builder --install ./build net.purrdata.PurrData.yml --force-clean --user

During development you can also run a build without installing it, like this:

    flatpak-builder --run build net.purrdata.PurrData.yml pd-l2ork

See the [Flatpak manual](http://docs.flatpak.org/en/latest/) for more information.
