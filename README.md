### Rayforge Flatpak
This is Flathub package of [Rayforge](https://github.com/barebaric/rayforge).

## Building
This package can be built locally by running
```
flatpak run org.flatpak.Builder --force-clean --sandbox --user --ccache --install-deps-from=flathub --repo=repo builddir com.barebaric.rayforge.yml
flatpak build-bundle repo rayforge.flatpak com.barebaric.rayforge
```

## Installation
```
flatpak --user install repo rayforge.flatpak
```
