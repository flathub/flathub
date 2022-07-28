# Bforartists Flatpak

Website: https://www.bforartists.de/

Source: https://github.com/Bforartists/Bforartists/

## Testing this Flatpak

### Build
```bash
cd de.bforartists.Bforartists
flatpak-builder --force-clean --install-deps-from=flathub build-dir de.bforartists.Bforartists.json
```

### Install and Run
```bash
flatpak-builder --user --install --force-clean build-dir de.bforartists.Bforartists.json
flatpak run de.bforartists.Bforartists
```

### Uninstalling
```bash
flatpak uninstall de.bforartists.Bforartists
```

## Updating Bforartists Version

### de.bforartists.Bforartists.json
- Update the Bforartists archive URL and SHA under `modules`->`bforartists`->`sources`.

### de.bforartists.Bforartists.appdata.xml
- Add a new release under `releases`. A description and URL is not required but recommended.

### A note on updating
This flatpak is based on the [Flathub Blender build files](https://github.com/flathub/org.blender.Blender). These files should periodically be rebased on newer versions of the Flathub Blender build files to stay in line with the latest changes.
