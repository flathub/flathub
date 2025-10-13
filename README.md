# com.fastmail.Fastmail

## Setup

Install `flatpak` if your distro doesn't include it.

https://flatpak.org/setup/

```bash
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install -y flathub org.flatpak.Builder
```

## Before publishing a new update

**Lint metadata manifest**

```bash
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream com.fastmail.Fastmail.metainfo.xml
```

**Create build**

```bash
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir com.fastmail.Fastmail.yml
```

**Preview store listing**

```bash
gnome-software --show-metainfo com.fastmail.Fastmail.metainfo.xml
```
