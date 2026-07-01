# TuxScale

AI-powered video upscaler for Linux. Utilizes Real-ESRGAN models and Vulkan compute to enhance video quality locally.

## Build

```bash
flatpak install -y flathub org.flatpak.Builder
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak run --command=flathub-build org.flatpak.Builder --install io.github.arrifat346afs.TuxScale.yml
```

## Run

```bash
flatpak run io.github.arrifat346afs.TuxScale
```

## Lint

```bash
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.arrifat346afs.TuxScale.yml
```

## Upstream

https://github.com/arrifat346afs/TuxScale
