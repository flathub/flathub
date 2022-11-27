# Gephi on Flatpak

Gephi (https://gephi.org/) on Flatpak using FreeDesktop SDK.

## How to build

Ensure `flatpak-builder` is installed

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install org.flathub.Builder
```

Build

```bash
flatpak run org.flatpak.Builder build-dir org.gephi.Gephi.yml --force-clean
```

Install from `build-dir` directory

```bash
flatpak run org.flatpak.Builder --user --install --force-clean build-dir org.gephi.Gephi.yml
```

Then run

```bash
flatpak run org.gephi.Gephi
```
