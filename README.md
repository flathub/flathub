Splash Flathub repository
=========================

To build the package manually:

```bash
flatpak-builder --ccache --repo=flatpak_repo build --force-clean org.splash.Splash.json
flatpak build-bundle flatpak_repo splash.flatpak org.splash.Splash
```
