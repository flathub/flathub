Splash Flathub repository
=========================

To build the package manually:

```bash
# Install dependencies
sudo apt install -y flatpak-builder
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freedesktop.Platform//19.08 org.freedesktop.Sdk//19.08

# Build the package
flatpak-builder --ccache --repo=flatpak_repo build --force-clean com.gitlab.sat_metalab.Splash.json
flatpak build-bundle flatpak_repo splash.flatpak com.gitlab.sat_metalab.Splash

# Install it
flatpak install --user splash.flatpak
```
