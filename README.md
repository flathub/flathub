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
