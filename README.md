This allows to create a [Flatpak](https://flatpak.org) universal Linux package. The application runs inside a sandbox.

## Build
```
flatpak-builder --force-clean build-dir com.lablicate.OpenChrom.yaml
```

### Test
```
flatpak-builder --user --install --force-clean build-dir com.lablicate.OpenChrom.yaml
flatpak run com.lablicate.OpenChrom
```
