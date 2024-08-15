# Build instructions

Download flatpak-pip-generator
`curl -o flatpak-pip-generator https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator`

Do a `python3 flatpak-pip-generator requests platformdirs, etc.` to generate a json file (keep this separate!).
Manually go to pypi and update the hashes and download links for PySide6 dependencies.


`flatpak-builder build-dir io.github.mak448a.Qtcord.yml`
`flatpak-builder --user --install --force-clean build-dir io.github.mak448a.Qtcord.yml`

## Resources
https://docs.flatpak.org/en/latest/first-build.html
https://docs.flatpak.org/en/latest/sandbox-permissions.html
