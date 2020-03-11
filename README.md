# Flatpak build for Basemark GPU

The build will download the pre-defined Basemark GPU archive from
basemark.com and create a flatpak that can be used to run the benchmark from it.

## Usage

1. Install depdencies
2. Run `flatpak-builder <build folder> com.basemark.BasemarkGPU.yaml`


## Installing dependencies

Install flatpak and flatpak-builder for your environment (see https://flatpak.org/ for more information)

1. flatpak install flathub org.freedesktop.Sdk//19.08
2. flatpak install flathub org.freedesktop.Platform//19.08
3. flatpak install flathub org.electronjs.Electron2.BaseApp//19.08