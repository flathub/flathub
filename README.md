# Pure Data packaged for Flatpak 

This is a Flatpak for [Pure Data](http://puredata.info/), an open source
visual programming language for multimedia.

This Flatpak uses the [Purr Data fork](https://github.com/agraef/purr-data/) of
Pure Data.

## How to build

This Flatpak uses the standard
[flatpak-builder](docs.flatpak.org/en/latest/flatpak-builder-command-reference.html)
tool to build.

Developers can run a command like the following to build the package from this repo:

    flatpak-builder ./build info.puredata.PurrData.yml --force-clean

This command tests that the build works as expected:

    flatpak-builder --run build info.puredata.contrib.PurrData.yml pd-l2ork
