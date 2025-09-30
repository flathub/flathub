# Clean old builds
rm -rf build-dir
#rm -rf .flatpak-builder

# Build but don’t install yet
flatpak-builder --force-clean build-dir io.github.DamnedAngel.msx-tile-forge.yaml --stop-at=msx-tile-forge

# Lint manifest
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.DamnedAngel.msx-tile-forge.yaml

# Build completely (install app files)
flatpak-builder --force-clean build-dir io.github.DamnedAngel.msx-tile-forge.yaml

# Lint appdir content
flatpak run --command=flatpak-builder-lint org.flatpak.Builder builddir build-dir

# Test run
flatpak-builder --run build-dir io.github.DamnedAngel.msx-tile-forge.yaml msxtileforge

