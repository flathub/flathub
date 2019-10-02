# [FL Studio](https://image-line.com/flstudio/)

> Digital Audio Workstation.

This is an **unofficial** Flatpak package for FL Studio.
Installer binaries are not packaged, but downloaded by the client.

## Status

| Arch  | Installs | Runs | Notes |
| ----- | -------- | ---- | ----- |
| 64bit | Yes      | Yes  | None  |

## Build & Install
### Repo
#### 64bit
```bash
flatpak-builder --arch=x86_64 --force-clean builds --repo=winepak com.imageline.flstudio.yml
flatpak --user install winepak com.imageline.flstudio
```
### Direct
#### 64bit
```bash
flatpak-builder --user --arch=x86_64 --force-clean --install builds com.imageline.flstudio.yml
```
## Run
```bash
flatpak run com.imageline.flstudio
```
