# Chirp Flatpak

A third party flatpak distribution of CHIRP.

## How to build

### 1. Install Flatpak Builder

Add flathub repo

```bash
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
```

Install builder
```bash
flatpak install org.flatpak.Builder
```

### 2. Build and Install Flatpak

```bash
flatpak run org.flatpak.Builder build --user --install --force-clean io.github.satisflux.chirp.yaml
```
