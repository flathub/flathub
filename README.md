# TheIDE

## Introduction

This repository acts as a recipe for building TheIDE as a flatpak package. TheIDE is an integrated development environment for the [U++ framework](https://www.ultimatepp.org/). This flatpak brings full experience. It means that after installation, users will be able to build applications using the U++ framework directly from TheIDE.

## Local Flatpak Developmenet

### Requiremenets

To build this flatpak localy, you need to install following package:
```
flatpak install org.flatpak.Builder
```

Additionaly, if you want you can install dependencies manualy, by running following commands:
```
flatpak install flathub org.gnome.Platform
flatpak install org.freedesktop.Sdk.Extension.llvm17
flatpak install org.freedesktop.Sdk.Extension.golang
```

### Building

To build flatpak locally run the following command in your terminal:
```
flatpak-builder --ccache --user --install --force-clean build-dir org.ultimatepp.TheIDE.yml
```

The `--ccache` parameter is higly recommended since it speedsup build time signficiently. Especially in situation when there is a need to build project several times.

Alternativly, if you want to execute full build with dependency installation run:
```
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/repo/screenshots --repo=repo build-dir org.ultimatepp.TheIDE.yml
```

In case if above command fails on install dependencies step try to use following command:
```
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

### Running

To run build flatpak simple run:
```
flatpak run org.ultimatepp.TheIDE
```

### Uninstalling

To uninstall please execute following command:
```
flatpak uninstall --delete-data org.ultimatepp.TheIDE
```

The `--delete-data` parameter can be skipped, however when there is a need to simulate running TheIDE for the first time, it is required. Also, for making absolute clean run, U++ framework sources that should be placed in `~/.local/src/upp` must be deleted. Without it the initial terminal asking about installing host dependencies will not appear.

## Known limitations

Below is the list of known problems of this flatpak:
- TheIDE can not play system sound (This requires modification to the liobrary. Will be done in the future)

## Essential linsk

If you want to learn more, here is the link to the essential links:
- [U++ framework official site](https://www.ultimatepp.org/)
- [U++ organization on GitHub](https://github.com/ultimatepp)
