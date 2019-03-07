# gov.cti.InVesalius

## Prerequisites

```bash
# Add flathub repo
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
# Install Gnome SDK and Runtime
flatpak install flathub org.gnome.Sdk//3.28
flatpak install flathub org.gnome.Platform//3.28
# Install GFortran  SDK to compile Scipy and Numpy
flatpak install flathub org.freedesktop.Sdk.Extension.gfortran-62
```

## Build

```bash
git clone https://github.com/tfmoraes/invesalius-flatpak.git
git submodule update --init
flatpak-builder build-dir gov.cti.InVesalius.json --force-clean --ccache --require-changes
```

## Testing

```bash
flatpak-builder --run build-dir gov.cti.InVesalius.json runner.sh
```
