# Download flatpak-pip-generator
curl -o flatpak-pip-generator https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator

Do a `python3 flatpak-pip-generator requests platformdirs` to generate info for that


flatpak-builder --user --install --force-clean build-dir io.github.mak448a.QTCord.yml
https://docs.flatpak.org/en/latest/first-build.html
https://docs.flatpak.org/en/latest/sandbox-permissions.html
