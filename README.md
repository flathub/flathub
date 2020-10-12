# Flatpak for openMSX

openMSX is an emulator for the MSX home computer system. MSX is an old
Z80-based family of home computers as an attempt to establish
a single standard in home computing similar to VHS in video.

The MSX standard has been designed by a company called ASCII in cooperation
with Microsoft which has provided a firmware version of its extended BASIC
(called "MicroSoft eXtended BASIC") for the machine, which explains the
MSX name.

In addition to emulating MSX, MSX2, MSX2+, MSX Turbo R and many of it's
peripherals, openMSX also supports emulating the ColecoVision game console
and the SpectraVideo SVI-318 and SVI-328 home computer systems.

## Installation

This Flatpak is available on [Flathub](https://flathub.org/apps/details/org.openmsx.openMSX).
After following the [Flatpak setup guide](https://flatpak.org/setup/), you can install it by entering the following
command in a terminal:

```bash
flatpak install flathub org.openmsx.openMSX -y
```

Once the Flatpak is installed, you can run openMSX or the Catapult launcher
using your desktop environment's application launcher or by running
`flatpak run org.openmsx.openMSX` in a terminal to run openMSX or
`flatpak run org.openmsx.openMSX openmsx-catapult` to run Catapult.

Note: by default the openMSX emulator only ships with the C-BIOS open-source
BIOS implementation. This BIOS is restricted to cartridge based MSX games. For
a full MSX or SVI emulation it is necessary to provide your own MSX and/or SVI
BIOS ROMs, which must be placed in the ~/.openMSX/share/systemroms directory.

## Updating

This Flatpak follows the latest stable openMSX version.
To update it, run the following command in a terminal:

```bash
flatpak update
```

## Limitations

- For security reasons, this Flatpak is sandboxed and only has access to the
  user's Home folder. You should place any files you need within openMSX in
  that folder (or in a subfolder).

Please [create an issue](https://github.com/flathub/org.openmsx.openMSX/issues/new) if you find any other limitations specific to flatpak that
should be documented here.

For general assistance with openMSX, visit [openmsx.org](https://openmsx.org).
## Configuration files

openMSX does not follow the freedesktop.org XDG Base Directory Specification.

User config files will be created under ~/.openMSX and ~/.Catapult after first launch.
To access it with a graphical file manager, you'll have to make hidden folders visible.

## Building from source

Install Git, follow the [flatpak-builder setup guide](https://docs.flatpak.org/en/latest/first-build.html) then enter the following commands in a terminal:

```bash
git clone --recursive https://github.com/flathub/org.openmsx.openMSX.git
cd org.openmsx.openMSX
flatpak install flathub org.freedesktop.Sdk//20.08 -y
flatpak-builder --force-clean --install --user -y builddir org.openmsx.openMSX.yaml
```

If all goes well, the Flatpak will be installed after building. You can then
run it using your desktop environment's application launcher.

You can speed up incremental builds by installing [ccache](https://ccache.dev/)
and specifying `--ccache` in the flatpak-builder command line (before `builddir`).
