# Demeter Flatpak
A flatpak repackaging of the Demeter XAS package by Bruce Ravel [Demeter Homepage](https://bruceravel.github.io/demeter/). This is an unofficial project and is **not** supported or maintained by the Demeter team. All questions and comments about this flatpak should stay within this repository.

## Development
This project began as a way to more easily install Demeter onto different machines and OS installs. It is a personal project.

This package was developed using the following scripts. It should be possible to build and install Demeter in this way. However, since this is now deployed to Flathub, I would recommend you simply download the container from Flathub.

## Installation
### Requirements
- `flatpak`
- `flatpak-builder` <br> This can be a prebuilt binary or built using `meson`. [Github link](https://github.com/flatpak/flatpak-builder)
- `org.freedesktop.Sdk 22.08` <br> This is the SDK for building the Demeter flatpak and the necessary runtime environment. <br> `flatpak install flathub org.freedesktop.Platform//22.08 org.freedesktop.Sdk//22.08` <br>

### Optional
- `flatpak-builder-tools` <br> Only required for modifying Perl libraries. This is required for generating the Perl dependencies. [Github link](https://github.com/flatpak/flatpak-builder-tools)

### Building the flatpak
Clone this repository and run `./build_flatpak.sh` from the `scripts/` directory.

The script will run `flatpak-builder`. The installation reaches out and downloads source archives and files for all the individual modules and builds them. The build process takes about 25 min on my laptop. 

## Scripts
`build_flatpak.sh` <br>
This script will build and install the Demeter flatpak. A full install takes about 25 min.

`make_perl_deps.sh` <br>
Demeter requires many Perl dependencies. This will take a list of Perl modules, add additional dependencies, and generate a JSON and YML file for downloading and installing these files into the flatpak.

`clean_flatpak.sh` <br>
During the build process, it was necessary to build and rebuild many times. This will delete the build folders, but leave the downloaded archives.

`purge_flatpak.sh` <br>
During the build process, it was necessary to build and rebuild many times. This will delete the build folders and the downloaded archives.

