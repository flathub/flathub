builder := "flatpak run org.flatpak.Builder"

_help:
    just --list

# Builds and install flatpak
build:
    {{builder}} build --install --user --force-clean com.chirpmyradio.chirp.yaml

# Run flatpak
run:
    flatpak run com.chirpmyradio.chirp

# Run flatpak linter
linter:
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.chirpmyradio.chirp.yaml
