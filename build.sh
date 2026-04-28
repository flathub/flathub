
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir systems.fracture.launcher.yml
echo "Running flatpak"
flatpak run systems.fracture.launcher
